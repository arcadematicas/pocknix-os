# Parches desactivados (renombrados a `.disabled`: el build solo aplica `*.patch`)

- `0044-clk-qcom-gcc-sm8750-Do-not-turn-off-PCIe-GDSCs...patch`
  Mantiene los GDSC del PCIe en **retention** durante suspend. armada #723 lo desactiva para
  que el PCIe se apague de verdad (full off) en s2idle y el SoC alcance CXSD/DDR.
  Riesgo: si al despertar el AOP/PCIe no vuelve, revertir (renombrar de nuevo a `.patch`).
  Referencia: armada PR #723 (SM8750 CXSD/DDR low power entry) y #746.
