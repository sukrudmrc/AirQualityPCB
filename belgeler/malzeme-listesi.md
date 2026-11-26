# Malzeme listesi

Bu listede fiyat yer almaz. Yaklaşık maliyet makalede verilmiştir (bkz. [Atıf](../README.md#atıf)).

## 1. Sensör istasyonu: modüller ve kart dışı parçalar

| Bileşen | Adet | Açıklama |
|---|---:|---|
| Nova Fitness SDS011 | 1 | PM2.5 / PM10 partikül sensörü, lazer saçılımı, UART, 5 V |
| MiCS-5524 kırılım kartı | 1 | CO / indirgeyici gaz için MOS sensör, analog çıkış, 5 V |
| SHT31 modülü | 1 | Sıcaklık ve bağıl nem sensörü, I²C, 3,3 V |
| Ebyte E220-900T22D | 1 | LLCC68 tabanlı LoRa modülü, 850,125–930,125 MHz, en çok 22 dBm, UART |
| LoRa anteni | 1 | 900 MHz bandı, 5 dBi, çok yönlü, SMA erkek |
| Devre kartı | 1 | `donanim/` klasöründeki tasarım, 2 katman, 100 × 50 mm |
| Güneş paneli | 1 | Bataryaları şarj etmek için |
| TP4056 şarj modülü | 1 | CC/CV Li-ion şarj ve koruma devresi |
| Li-ion hücre | 2 | Paralel bağlı, 3,7 V anma gerilimi |
| Yükseltici (boost) dönüştürücü | 1 | 5 V çıkış (SDS011 ve MiCS-5524 için), karttaki J2 girişine |
| Düşürücü (buck) dönüştürücü | 1 | 3,3 V çıkış (STM32 ve SHT31 için), karttaki J1 girişine |
| Proje kutusu | 1 | Dış ortam için muhafaza |

## 2. Devre kartı bileşenleri

Tam liste KiCad şematiğinden üretilmiştir: [`donanim/malzeme-listesi-kart.csv`](../donanim/malzeme-listesi-kart.csv). Özet:

| Referans | Değer | Kılıf | Adet |
|---|---|---|---:|
| U2 | STM32F103C8T6 | LQFP-48 | 1 |
| Y1 | Kristal | SMD 5032, 4 bacak | 1 |
| C1, C4, C8, C9, C11, C12, C14–C19 | 100 nF | 0603 | 12 |
| C2, C3 | 22 pF | 0603 | 2 |
| C7 | 4,7 µF | 1206 | 1 |
| C5, C6, C10, C13 | 1000 µF 25 V | Radyal, Ø10 mm | 4 |
| L1, L2 | 100 µH | Radyal, Ø10 mm | 2 |
| R1, R2 | 10 kΩ | 0603 | 2 |
| R3, R4, R5 | 100 Ω | 0603 | 3 |
| R6 | 0 Ω | 0603 | 1 |
| D1, D2, D3 | LED (hata ayıklama, 3,3 V, 5 V) | 0805 | 3 |
| RESET1 | Buton | DTS-3 | 1 |
| J1, J2 | Güç girişi (3,3 V, 5 V) | Phoenix MC 3,5 mm, 2 pin | 2 |
| J3, pm2.5, SHT31 | Sensör soketleri | 2,54 mm dişi, 1×4 | 3 |
| LoRa1 | E220 soketi | 2,54 mm dişi, 1×7 | 1 |
| SWD1, J4 | Programlama / bağlantı | 2,54 mm erkek, 1×4 | 2 |
| J5 | Sürgülü anahtar | SSL-12F20-G4 | 1 |
| TP1–TP7 | Test noktaları | Ped | 7 |

## 3. Ağ geçidi

| Bileşen | Adet | Açıklama |
|---|---:|---|
| ESP32 geliştirme kartı | 1 | Wi-Fi, Arduino-ESP32 çekirdeği |
| Ebyte E220-900T22D | 1 | İstasyondakiyle aynı modül |
| LoRa anteni | 1 | 900 MHz, 5 dBi, SMA |

Bir ağ geçidi birden fazla istasyona hizmet verebilir.

## Geliştirme araçları

- ST-LINK/V2 programlayıcı (STM32'ye SWD üzerinden yazılım yüklemek için)
- USB–UART dönüştürücü (CP2102 vb.; E220 yapılandırması ve seri port kayıtları için)
