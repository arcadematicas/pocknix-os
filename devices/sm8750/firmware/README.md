# Firmware overrides — AYN Odin 3 (SM8750)

Blobs here are applied to the image rootfs **after** the ROCKNIX firmware overlay
(`install_firmware()` in `scripts/build-image.sh`), so they win over whatever the
overlay ships. Paths are relative to `/usr/lib/firmware/`.

## `qcom/sm8750/adsp.mbn` + `qcom/sm8750/adsp_dtb.mbn`

The AYN Odin 3 ADSP **charger** firmware. The device DTS
(`kernel/sm8750/dts/qcom/cq8725s-ayn-odin3.dts`) sets
`firmware-name = "qcom/sm8750/adsp.mbn"`, so the kernel loads these paths.

`adsp_dtb.mbn` carries the **battery-authentication config**
(`batt_auth_cfg`, `batt-auth-public-key`, `batt-unauth-charging-action`,
`en-batt-auth`). The blob Pocknix used to get (from the ROCKNIX overlay / ALARM)
lacks it, so the ADSP charger firmware could not authenticate the battery, fell
into **TEST MODE (state 9)** and the battery never charged under Linux.

These two files are the ones ArmadaOS ships at
`qcom/sm8750/ayn/odin3/` (its DTS uses the board-namespaced path). Copied to
`qcom/sm8750/` here so the current DTS picks them up.

| file | size | md5 |
|---|---|---|
| `adsp.mbn` | 21907848 | `6cfcbbb80b956ddad76950c038ea1a3e` |
| `adsp_dtb.mbn` | 167736 | `d88d7ecbba78ecacb13adcc7bcbe131d` |

Verified on-device: `battery status=Charging`, `qcom-battmgr-usb online=1`,
`ucsi 3A`, and the charger firmware ulog no longer reports "Test mode".
See `pocknix-odin3-support/BATTERY-ISSUE.md` and
`pocknix-odin3-support/kernel/build-config/FIX-CARGA-BATERIA.md`.
