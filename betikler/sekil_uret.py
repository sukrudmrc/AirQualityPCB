"""Makaledeki sensör test şeklini veri/ klasöründeki kayıtlardan üretir.

Çalıştırma (depo kök dizininden):
    pip install matplotlib
    python betikler/sekil_uret.py

Girdiler:
    veri/sht31_oturum1.csv, veri/sht31_oturum2.csv, veri/sht31_oturum3.csv  (SHT31)
    veri/mics5524_co.csv                                                    (MiCS-5524)
    veri/sds011_pm.csv                                                      (SDS011)

Çıktılar:
    belgeler/gorseller/sensor_testleri.png ve .pdf
"""
import csv
import re
import statistics as st
from datetime import datetime
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

KOK = Path(__file__).resolve().parents[1]
VERI = KOK / "veri"
CIKTI = KOK / "belgeler" / "gorseller" / "sensor_testleri"

plt.rcParams.update({
    "font.family": "serif",
    "font.size": 8,
    "axes.linewidth": 0.6,
    "lines.linewidth": 0.9,
    "legend.frameon": False,
})
virgul = FuncFormatter(lambda v, _: f"{v:g}".replace(".", ","))


def sht31_oku(dosya):
    satirlar = []
    for r in list(csv.reader(open(dosya, encoding="utf8")))[1:]:
        try:
            t = datetime.strptime(r[0], "%Y-%m-%d %H:%M:%S")
            T = float(re.sub(r"[^0-9.\-]", "", r[1]))
            H = float(re.sub(r"[^0-9.\-]", "", r[2]))
        except (ValueError, IndexError):
            continue
        satirlar.append((t, T, H))
    t0 = satirlar[0][0]
    return [((t - t0).total_seconds() / 60, T, H) for t, T, H in satirlar]


def zaman_sabiti(veri):
    """Basamak yanıtında değişimin %63,2'sine ulaşma süresi (dk)."""
    T0 = veri[0][1]
    son = [T for m, T, _ in veri if m >= veri[-1][0] - 5]
    Tson = st.mean(son)
    hedef = T0 - 0.632 * (T0 - Tson)
    for m, T, _ in veri:
        if T <= hedef:
            return m, T0, Tson
    return None, T0, Tson


def co_oku(dosya):
    idx, v = [], []
    for satir in open(dosya, encoding="utf8"):
        m = re.findall(r'"\s*([0-9.]+)\s*"', satir)
        if len(m) >= 2:
            deger = float(m[-1])
            if 0 < deger < 5:  # 5 V üstü değerler seri port bozulması (tek örnek)
                idx.append(int(float(m[0])))
                v.append(deger)
    return idx, v


def pm_oku(dosya):
    with open(dosya, encoding="utf8") as f:
        return [(float(r["pm25"]), float(r["pm10"])) for r in csv.DictReader(f)]


def main():
    fig, ax = plt.subplots(1, 3, figsize=(7.2, 2.2))

    # (a) SHT31: üç oturum, iç ortamdan dış ortama geçiş
    renk = ["#1f4e79", "#c55a11", "#548235"]
    print("SHT31 oturumları:")
    for i, ad in enumerate(["sht31_oturum1.csv", "sht31_oturum2.csv", "sht31_oturum3.csv"]):
        veri = sht31_oku(VERI / ad)
        tau, T0, Tson = zaman_sabiti(veri)
        print(f"  {ad}: n={len(veri)}, süre={veri[-1][0]:.1f} dk, "
              f"T0={T0:.2f}, Tson={Tson:.2f}, tau63={tau:.2f} dk")
        m = [x[0] for x in veri if x[0] <= 50]
        ax[0].plot(m, [x[1] for x in veri if x[0] <= 50], color=renk[i],
                   label=f"Oturum {i + 1}")
    ax[0].set_xlabel("Süre (dk)")
    ax[0].set_ylabel("Sıcaklık (°C)")
    ax[0].set_title("(a) SHT31", fontsize=8)
    ax[0].legend(fontsize=7)

    # (b) MiCS-5524 çıkış gerilimi
    idx, v = co_oku(VERI / "mics5524_co.csv")
    saat = [i / 3600 for i in idx]  # örnekleme periyodu 1 s (zamanlayıcı kesmesi)
    pencere = 300
    ort = [st.mean(v[k:k + pencere]) for k in range(0, len(v) - pencere, pencere)]
    ort_saat = [saat[k + pencere // 2] for k in range(0, len(v) - pencere, pencere)]
    ax[1].plot(saat, v, color="#bfbfbf", linewidth=0.3, label="Ham")
    ax[1].plot(ort_saat, ort, color="#1f4e79", label="5 dk ortalama")
    ax[1].set_xlabel("Süre (saat)")
    ax[1].set_ylabel("Çıkış gerilimi (V)")
    ax[1].set_title("(b) MiCS-5524", fontsize=8)
    ax[1].set_ylim(0, 1.6)
    ax[1].legend(fontsize=7, loc="upper right")
    print(f"MiCS-5524: n={len(v)}, süre={saat[-1] - saat[0]:.1f} saat, "
          f"ort={st.mean(v):.3f} V, ss={st.pstdev(v):.3f} V, "
          f"min={min(v):.3f}, max={max(v):.3f}")

    # (c) SDS011
    pm = pm_oku(VERI / "sds011_pm.csv")
    n = list(range(1, len(pm) + 1))
    ax[2].plot(n, [p[0] for p in pm], color="#1f4e79", label="PM2.5")
    ax[2].plot(n, [p[1] for p in pm], color="#c55a11", label="PM10")
    ax[2].set_xlabel("Örnek numarası")
    ax[2].set_ylabel("Derişim (µg/m³)")
    ax[2].set_title("(c) SDS011", fontsize=8)
    ax[2].legend(fontsize=7)
    a = [p[0] for p in pm]
    b = [p[1] for p in pm]
    print(f"SDS011: n={len(pm)}, PM2.5 ort={st.mean(a):.2f} ss={st.pstdev(a):.2f} "
          f"[{min(a)}, {max(a)}], PM10 ort={st.mean(b):.2f} ss={st.pstdev(b):.2f} "
          f"[{min(b)}, {max(b)}], PM2.5/PM10={st.mean(x / y for x, y in pm):.3f}")

    for e in ax:
        e.yaxis.set_major_formatter(virgul)
        e.xaxis.set_major_formatter(virgul)
        e.tick_params(width=0.6, length=2.5)
    fig.tight_layout(pad=0.4, w_pad=1.0)
    for uzanti in (".png", ".pdf"):
        fig.savefig(CIKTI.with_suffix(uzanti), dpi=200)
        print("Kaydedildi:", CIKTI.with_suffix(uzanti))


if __name__ == "__main__":
    main()
