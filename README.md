# LoRa Tabanlı Hava Kalitesi İzleme İstasyonu

Bu proje, düşük maliyetli, güneş enerjisiyle çalışan ve tamamen açık kaynak bir hava kalitesi izleme istasyonudur. İstasyon PM2.5, PM10, CO, sıcaklık ve bağıl nem ölçer. Ölçümler LoRa ile bir ağ geçidine, oradan da herkese açık bir ThingSpeak kanalına iletilir.

[![Donanım lisansı: CERN-OHL-P-2.0](https://img.shields.io/badge/donan%C4%B1m-CERN--OHL--P--2.0-blue)](LICENSES/CERN-OHL-P-2.0.txt)
[![Yazılım lisansı: MIT](https://img.shields.io/badge/yaz%C4%B1l%C4%B1m-MIT-green)](LICENSES/MIT.txt)
[![Veri ve belge lisansı: CC BY 4.0](https://img.shields.io/badge/veri%20%26%20belge-CC%20BY%204.0-lightgrey)](LICENSES/CC-BY-4.0.txt)
![KiCad 9](https://img.shields.io/badge/KiCad-9.0-314cb0)
![STM32CubeIDE](https://img.shields.io/badge/STM32CubeIDE-STM32F103-03234b)
![Arduino-ESP32](https://img.shields.io/badge/Arduino-ESP32-00979d)

<p align="center">
  <img src="belgeler/gorseller/pcb_3b.png" alt="Devre kartının KiCad 3B görünümü" width="49%">
  <img src="belgeler/gorseller/sistem.jpg" alt="Bileşenleri lehimlenmiş sensör istasyonu" width="49%">
</p>

> Bu depo, *"Düşük Maliyetli, Güneş Enerjili ve Açık Kaynaklı LoRa Tabanlı Hava Kalitesi İzleme İstasyonu"* başlıklı makalenin tamamlayıcı deposudur (Ş. Demirci, İ. Baştürk; Manisa Celal Bayar Üniversitesi). Makale yayımlandığında künyesi [Atıf](#atıf) bölümüne eklenecektir.

## İçindekiler

- [Öne çıkanlar](#öne-çıkanlar)
- [Sistem mimarisi](#sistem-mimarisi)
- [Donanım](#donanım)
- [Depo yapısı](#depo-yapısı)
- [Hızlı başlangıç](#hızlı-başlangıç)
- [LoRa veri paketi](#lora-veri-paketi)
- [Canlı veri](#canlı-veri)
- [Test verileri](#test-verileri)
- [Bilinen sınırlılıklar](#bilinen-sınırlılıklar)
- [Gelecek çalışmalar](#gelecek-çalışmalar)
- [Atıf](#atıf)
- [Lisans](#lisans)
- [Proje ekibi](#proje-ekibi)

## Öne çıkanlar

- **Şebekeden bağımsız:** Güneş paneli, TP4056 şarj devresi ve iki Li-ion hücreyle çalışır.
- **Altyapı gerektirmeyen haberleşme:** E220-900T22D modülleri noktadan noktaya LoRa bağlantısı kurar; LoRaWAN ağ sunucusuna ihtiyaç yoktur.
- **Özgün devre kartı:** STM32F103C8 tabanlı 2 katmanlı kart, KiCad ile tasarlanmıştır (100 × 50 mm).
- **Düşük gürültülü besleme:** Anahtarlamalı dönüştürücülerin gürültüsü π tipi LC filtreyle bastırılır, böylece analog CO ölçümü korunur.
- **Herkese açık veri:** Ölçümler ESP32 ağ geçidi üzerinden MQTT ile ThingSpeak'e gönderilir.
- **Yeniden üretilebilir:** Kart, yazılım, malzeme listesi, test verileri ve şekil betiği bu depodadır.

## Sistem mimarisi

```mermaid
flowchart LR
  subgraph IST["Sensör istasyonu"]
    direction TB
    PV["Güneş paneli"] --> CHG["TP4056 şarj"] --> BAT["2 × Li-ion"]
    BAT --> BST["Boost 5 V"] & BCK["Buck 3,3 V"]
    BST & BCK --> FLT["π LC filtre"]
    SDS["SDS011<br/>PM2.5 / PM10"] -- UART --> MCU["STM32F103C8"]
    MICS["MiCS-5524<br/>CO"] -- ADC --> MCU
    SHT["SHT31<br/>Sıcaklık / nem"] -- I²C --> MCU
    FLT -.-> MCU
    MCU -- UART --> E1["E220-900T22D"]
  end
  subgraph GW["Ağ geçidi"]
    E2["E220-900T22D"] -- UART --> ESP["ESP32"]
  end
  E1 <-.->|"LoRa: uyandırma / 13 baytlık paket"| E2
  ESP -- "Wi-Fi · MQTT" --> TS["ThingSpeak<br/>mqtt3.thingspeak.com"]
```

Ağ geçidi 60 saniyede bir istasyona bir uyandırma baytı gönderir. İstasyon bu baytı UART kesmesiyle alır ve şu adımları sırayla yürütür:

1. Her sensörden 10 örnek alır (SHT31 → MiCS-5524 → SDS011).
2. Örneklerin ortalamasını hesaplar.
3. Sonucu 13 baytlık bir paket olarak geri gönderir.

Ağ geçidi paketi doğrular ve ThingSpeak'e yayımlar.

## Donanım

| Bileşen | Ölçülen | Arayüz | STM32 pinleri |
|---|---|---|---|
| [SDS011](https://www.nova-fitness.com/) | PM2.5, PM10 (0–999,9 µg/m³) | UART, 9600 bit/s | USART2 (PA2/PA3) |
| [SHT31](https://sensirion.com/products/catalog/SHT31-DIS-B) | Sıcaklık, bağıl nem | I²C | I2C1 (PB6/PB7) |
| [MiCS-5524](https://www.sgxsensortech.com/) | CO / indirgeyici gazlar (eğilim göstergesi) | ADC, 12 bit | ADC1_IN8 (PB0) |
| [E220-900T22D](https://www.cdebyte.com/products/E220-900T22D) | LoRa haberleşme (LLCC68) | UART, 9600 bit/s | USART1 (PA9/PA10) |

- Şema (PDF): [`donanim/sema.pdf`](donanim/sema.pdf)
- KiCad projesi: [`donanim/kicad/`](donanim/kicad/)
- Üretim dosyaları (Gerber ve delik dosyaları): [`donanim/gerber/`](donanim/gerber/)
- Malzeme listesi: [`belgeler/malzeme-listesi.md`](belgeler/malzeme-listesi.md)

Ayrıntılar için bkz. [`donanim/README.md`](donanim/README.md).

## Depo yapısı

```
├── donanim/
│   ├── kicad/                  KiCad 9 projesi (şematik, kart, özel kütüphaneler)
│   ├── gerber/                 Üretici için Gerber ve delik dosyaları
│   ├── sema.pdf                Devre şeması
│   └── malzeme-listesi-kart.csv
├── yazilim/
│   ├── istasyon-stm32/         STM32CubeIDE projesi (sensör istasyonu)
│   └── ag-gecidi-esp32/        Arduino taslağı (LoRa → Wi-Fi/MQTT ağ geçidi)
├── veri/                       Sensör birim testlerinin ham kayıtları (CSV)
├── betikler/                   Şekil üretme ve seri port kayıt betikleri (Python)
├── belgeler/                   Malzeme listesi ve görseller
├── LICENSES/                   Lisans metinleri
├── CITATION.cff
└── LICENSE
```

## Hızlı başlangıç

### 1. Kartı ürettirmek

[`donanim/gerber/`](donanim/gerber/) klasöründeki dosyaları sıkıştırıp bir PCB üreticisine yükleyin. Kart özellikleri: 2 katman, FR-4, 1,6 mm, 100 × 50 mm. Bileşenler elle lehimlenebilecek boyuttadır (0603 / 0805 / LQFP-48).

### 2. İstasyon yazılımı (STM32)

1. [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) ile `yazilim/istasyon-stm32` klasörünü içe aktarın: *File → Import → Existing Projects into Workspace*.
2. `AllSesnsorRead.ioc` dosyasını açıp **Generate Code** komutunu çalıştırın. Bu adım HAL sürücülerini (`Drivers/`, STM32Cube FW_F1 V1.8.6) indirir; sürücüler depoya konmamıştır.
3. Projeyi derleyin ve kartın SWD bağlantısı üzerinden ST-LINK/V2 ile yükleyin.

### 3. Ağ geçidi yazılımı (ESP32)

1. Arduino IDE'ye ESP32 çekirdeğini ve **PubSubClient** kütüphanesini kurun.
2. `yazilim/ag-gecidi-esp32/ag_gecidi_esp32/secrets.example.h` dosyasını aynı klasöre `secrets.h` adıyla kopyalayın. İçine Wi-Fi bilgilerinizi ve ThingSpeak MQTT cihaz bilgilerinizi yazın.
3. `ag_gecidi_esp32.ino` dosyasını ESP32'ye yükleyin. E220 modülü UART2'ye bağlanır: ESP32 RX = GPIO16, TX = GPIO17. M0 ve M1 LOW olmalıdır; ayrıntı için bkz. [`yazilim/README.md`](yazilim/README.md).

### 4. LoRa modüllerinin yapılandırılması

İki E220 modülü, üreticinin yapılandırma yazılımıyla aşağıdaki gibi ayarlanır:

| Parametre | Değer |
|---|---|
| UART | 9600 bit/s, 8N1 |
| İletim kipi | Sabit adresli (fixed transmission) |
| Hava veri hızı | 62,5 kbit/s |
| Adres | 0x0001 (istasyon), 0x0002 (ağ geçidi) |
| Kanal | Bkz. [Bilinen sınırlılıklar](#bilinen-sınırlılıklar) |

## LoRa veri paketi

| Bayt | İçerik | Kodlama |
|---|---|---|
| 0 | Başlangıç baytı | `0xAA` |
| 1–2 | PM2.5 (µg/m³) | 16 bit, düşük bayt önce, ×10 |
| 3–4 | PM10 (µg/m³) | 16 bit, düşük bayt önce, ×10 |
| 5–6 | CO kanalı | 16 bit, düşük bayt önce, ×10 |
| 7–8 | Sıcaklık (°C) | 16 bit, düşük bayt önce, ×10 |
| 9–10 | Bağıl nem (%) | 16 bit, düşük bayt önce, ×10 |
| 11 | Sağlama baytı | (b₁ + … + b₁₀) mod 256 |
| 12 | Bitiş baytı | `0xAB` |

Ağ geçidi değerleri ThingSpeak alanlarına şöyle yazar: `field1` PM2.5, `field2` PM10, `field3` CO, `field4` sıcaklık, `field5` bağıl nem.

## Canlı veri

İstasyonun verileri herkese açık ThingSpeak kanalında yayımlanır: **[thingspeak.mathworks.com/channels/2948169](https://thingspeak.mathworks.com/channels/2948169)**

<p align="center">
  <img src="belgeler/gorseller/thingspeak.png" alt="Bütünleşik sistem testinde ThingSpeak'e ulaşan veriler" width="80%">
</p>

## Test verileri

Makaledeki sensör birim testlerinin ham kayıtları [`veri/`](veri/) klasöründedir. Makaledeki şekil şu komutlarla yeniden üretilebilir:

```bash
pip install matplotlib
python betikler/sekil_uret.py
```

<p align="center">
  <img src="belgeler/gorseller/sensor_testleri.png" alt="SHT31, MiCS-5524 ve SDS011 birim test sonuçları" width="100%">
</p>

Dosyaların içeriği ve ölçüm koşulları için bkz. [`veri/README.md`](veri/README.md).

## Bilinen sınırlılıklar

Bu sürüm bir prototiptir. Aşağıdaki konular açıkça belirtilmektedir:

- **Ölçek uyuşmazlığı:** Ağ geçidi her değeri 10'a bölüyor (×10 kodlama). Ancak istasyon yazılımı PM değerlerini zaten 10'a bölünmüş tamsayı (µg/m³) olarak, SHT31 değerlerini ham sensör sayımı olarak, CO değerini ise ham ADC sayımı olarak gönderiyor. Paket biçimi makalede tanımlandığı gibidir; istasyon yazılımının bu biçime uydurulması gerekiyor.
- **Uyku kipi:** Ölçüm döngüleri arasında SLEEP kipine geçiş ayrı bir test yazılımında doğrulandı, ancak bütünleşik yazılımda henüz etkin değil.
- **Kalibrasyon yok:** Sensörler bir referans cihazla karşılaştırılmadı. MiCS-5524 kanalı nicel bir CO derişimi olarak değil, eğilim göstergesi olarak yorumlanmalıdır.
- **LoRa kanalı:** Yazılımlarda kanal `0x02` tanımlı. E220-900 modülünde bu, 852,125 MHz'e karşılık gelir. Türkiye'de lisanssız kısa menzilli cihazlar için 863–870 MHz bandı kullanılmalıdır (ör. kanal 18 = 868,125 MHz). Sahada kullanmadan önce kanal ayarını yerel düzenlemelere göre değiştirin.
- **Menzil:** LoRa bağlantısı yalnızca yaklaşık 227 m görüş hattında sınandı. RSSI/SNR ve paket teslim oranı kaydedilmedi.

## Gelecek çalışmalar

- İstasyon yazılımında ölçek uyuşmazlığının giderilmesi ve uyku kipinin etkinleştirilmesi
- Referans istasyonla yan yana kalibrasyon
- Uzun süreli saha testi, enerji bütçesi ve paket teslim oranı ölçümleri
- ThingSpeak bağımlılığını kaldırmak için açık kaynak bir sunucu yığını (MQTT aracısı, zaman serisi veritabanı, Grafana)

## Atıf

Bu çalışmayı kullanırsanız lütfen makaleye atıf yapın. GitHub'daki **"Cite this repository"** düğmesi [`CITATION.cff`](CITATION.cff) dosyasındaki bilgileri kullanır.

```bibtex
@misc{demirci_lora_hava_kalitesi,
  author       = {Demirci, Şükrü and Baştürk, İlhan},
  title        = {Düşük Maliyetli, Güneş Enerjili ve Açık Kaynaklı LoRa Tabanlı
                  Hava Kalitesi İzleme İstasyonu},
  howpublished = {\url{https://github.com/sukrudmrc/lora-hava-kalitesi-istasyonu}},
  note         = {Makale değerlendirme aşamasındadır}
}
```

## Lisans

Bu depo, içerik türüne göre farklı açık lisanslar kullanır:

| İçerik | Lisans |
|---|---|
| Donanım (`donanim/`) | [CERN-OHL-P-2.0](LICENSES/CERN-OHL-P-2.0.txt) |
| Yazılım (`yazilim/`, `betikler/`) | [MIT](LICENSES/MIT.txt) |
| Veri, görseller ve belgeler (`veri/`, `belgeler/`, README dosyaları) | [CC BY 4.0](LICENSES/CC-BY-4.0.txt) |

STM32CubeMX'in ürettiği dosyalar ve STMicroelectronics başlık dosyaları kendi lisans koşullarına tabidir. Ayrıntılar için bkz. [`LICENSE`](LICENSE).

## Proje ekibi

- **Şükrü Demirci:** donanım ve yazılım geliştirme, testler
- **Eren Gültekin:** tasarım projesi ekip üyesi
- **Doç. Dr. İlhan Baştürk:** danışman

Manisa Celal Bayar Üniversitesi, Mühendislik ve Doğa Bilimleri Fakültesi, Elektrik-Elektronik Mühendisliği Bölümü

<!-- Teşekkür: TÜBİTAK 2209-A desteği teyit edildiğinde buraya eklenecek. -->
