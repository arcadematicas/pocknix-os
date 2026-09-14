import { ModalRoot, ToggleField } from "@decky/ui";
import { useEffect, useState } from "react";
import { getConfig, makoStatus, saveTweaks } from "../backend";
import { fexSteamString, makoEnabledIn, readLaunchOptions, syncFexLaunchOption, syncMakoLaunchOption } from "../lib/launchOptions";
import { clone } from "../lib/util";
import type { Config, MakoStatus } from "../types";
import { ConfigSection } from "./ConfigSection";
import { PerfFields, TweakFields } from "./GameFields";

/** Standalone per-game settings, opened from the library context menu. Saves on each change
 *  (no QAM debounce lifecycle here; the modal has an explicit close). */
export function GameSettingsModal({ appid, name, closeModal }: { appid: string; name: string; closeModal?: () => void }) {
  const [config, setConfig] = useState<Config | null>(null);
  // MAKO is not part of the tweaks blob: it lives entirely in the game's launch options,
  // which Steam owns. Read the string back so the toggle reflects the real state rather
  // than a shadow copy the two UIs could disagree about.
  const [mako, setMako] = useState<MakoStatus | null>(null);
  const [makoOn, setMakoOn] = useState(false);
  useEffect(() => {
    getConfig()
      .then(setConfig)
      .catch(() => closeModal?.());
    makoStatus().then(setMako).catch(() => {});
    readLaunchOptions(appid)
      .then((options) => setMakoOn(options ? makoEnabledIn(options) : false))
      .catch(() => {});
  }, []);
  if (!config) return <ModalRoot closeModal={closeModal}>Loading…</ModalRoot>;

  const gameSettings = config.tweaks.games[appid] || {};
  const enabled = gameSettings.enabled === true;
  const values = enabled ? { ...config.tweaks.global, ...gameSettings } : config.tweaks.global;
  const update = (mutate: (next: Config) => void) => {
    const next = clone(config);
    mutate(next);
    setConfig(next);
    saveTweaks(next.tweaks).catch(() => {});
  };
  const patch = (fields: Record<string, any>) =>
    update((next) => {
      const existing = next.tweaks.games[appid] || {};
      next.tweaks.games[appid] = { ...existing, enabled: true, name, ...fields };
    });

  return (
    <ModalRoot closeModal={closeModal}>
      <div style={{ fontWeight: 600, marginBottom: "8px" }}>{name || `App ${appid}`}</div>
      {mako?.installed ? (
        <ToggleField
          label="MAKO (Lossless Scaling frame generation)"
          checked={makoOn}
          onChange={(on) => {
            setMakoOn(on);
            // Steam owns the string, so roll the toggle back if the write is refused.
            syncMakoLaunchOption(appid, on).catch(() => setMakoOn(!on));
          }}
        />
      ) : null}
      <ToggleField
        label="Use Per-Game Settings"
        checked={enabled}
        onChange={(on) => {
          update((next) => {
            next.tweaks.games[appid] = { ...(next.tweaks.games[appid] || {}), enabled: on, name };
          });
          const profile = on ? String(gameSettings.fexProfile ?? config.tweaks.global.fexProfile ?? "") : "";
          syncFexLaunchOption(appid, fexSteamString(profile, config.fexProfiles));
        }}
      />
      {enabled ? (
        <>
          <PerfFields values={values} patch={patch} />
          <TweakFields config={config} appid={appid} values={values} patch={patch} />
          <ConfigSection game={{ appid, name }} reload={() => getConfig().then(setConfig).catch(() => {})} />
        </>
      ) : null}
    </ModalRoot>
  );
}
