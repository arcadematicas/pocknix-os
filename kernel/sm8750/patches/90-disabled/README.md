# Parches desactivados (renombrados a `.disabled`: el build solo aplica `*.patch`)

- `0044-clk-qcom-gcc-sm8750-Do-not-turn-off-PCIe-GDSCs...patch`
  Mantiene los GDSC del PCIe en **retention** durante suspend. armada #723 lo desactiva para
  que el PCIe se apague de verdad (full off) en s2idle y el SoC alcance CXSD/DDR.
  Riesgo: si al despertar el AOP/PCIe no vuelve, revertir (renombrar de nuevo a `.patch`).
  Referencia: armada PR #723 (SM8750 CXSD/DDR low power entry) y #746.
- `0521-arm64-dts-qcom-sm8750-add-a-pcie-suspend-opp.patch`
  Añadía `opp-suspend-1` (floor de 1000 kBps) al PCIe: mantiene el voto de bus (SN4) en el
  sleep set a costa de **CX shutdown y DDR low power**. armada lo quitó junto con 0044
  (`2764b29`, `2f280a5`): sin ambos, el SoC despierta habiendo estado en CX shutdown todo
  el sleep y el WCN7860 vuelve porque pwrctrl lo apaga y el link re-entrena.
