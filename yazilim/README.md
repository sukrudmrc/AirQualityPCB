# Yazılım

| Klasör | Platform | Görev |
|---|---|---|
| [`istasyon-stm32/`](istasyon-stm32/) | STM32F103C8, STM32CubeIDE, HAL | Sensörleri okur, ortalama alır, 13 baytlık paketi LoRa ile gönderir |
| [`ag-gecidi-esp32/`](ag-gecidi-esp32/) | ESP32, Arduino | İstasyonu uyandırır, paketi doğrular ve ThingSpeak'e MQTT ile yayımlar |

## İstasyon yazılımı (STM32)

Yazılım bir durum makinesi olarak kurgulanmıştır (`Core/Src/main.c`):

```
STATE_Wait ──(LoRa'dan UART kesmesi)──► SHT31_DATA_READ ──► CO_DATA_READ ──► PM_DATA_READ
     ▲                                                                            │
     └──── CLEAR_BUFFER ◄──── SEND_LORA ◄──── MAKE_AVARAGE ◄────────────────────┘
```

| Durum | İşlem |
|---|---|
| `STATE_Wait` | Ağ geçidinden uyandırma baytı beklenir. Bu sırada hata ayıklama LED'i yanıp söner |
| `SHT31_DATA_READ` | I²C üzerinden 10 sıcaklık/nem örneği alınır |
| `CO_DATA_READ` | MiCS-5524 çıkışından ADC ile 10 örnek alınır |
| `PM_DATA_READ` | SDS011'den doğrulanmış 10 çerçeve okunur |
| `MAKE_AVARAGE` | Her büyüklüğün ortalaması hesaplanır |
| `SEND_LORA` | Paket oluşturulur, sağlama baytı eklenir ve gönderilir |
| `CLEAR_BUFFER` | Tamponlar sıfırlanır |

**Çevre birimleri** (`AllSesnsorRead.ioc`):

| Birim | Pinler | Bağlantı |
|---|---|---|
| USART1, 9600 bit/s | PA9 / PA10 | E220 LoRa modülü |
| USART2, 9600 bit/s | PA2 / PA3 | SDS011 |
| I2C1 | PB6 / PB7 | SHT31 |
| ADC1 kanal 8 | PB0 | MiCS-5524 |
| GPIO | PB13 | Hata ayıklama LED'i |
| SWD | PA13 / PA14 | Programlama |

**Derleme:**

1. STM32CubeIDE'de *File → Import → Existing Projects into Workspace* ile `istasyon-stm32` klasörünü seçin.
2. `AllSesnsorRead.ioc` dosyasını açıp *Generate Code* komutunu çalıştırın. HAL ve CMSIS sürücüleri (`Drivers/`) depoda yoktur; bu adımda STM32Cube FW_F1 V1.8.6 paketinden üretilir. Kullanıcı kodu `USER CODE` bölümlerinde olduğu için kod üretimi sırasında korunur.
3. *Build* ile derleyin ve ST-LINK/V2 ile yükleyin.

## Ağ geçidi yazılımı (ESP32)

`ag-gecidi-esp32/ag_gecidi_esp32/ag_gecidi_esp32.ino` şu döngüyü yürütür:

1. Wi-Fi'ye ve ThingSpeak MQTT aracısına (`mqtt3.thingspeak.com:1883`) bağlanır.
2. 60 saniyede bir istasyon adresine (0x0001) bir uyandırma mesajı gönderir.
3. 13 baytlık yanıtı alır; başlangıç baytını, bitiş baytını ve sağlama baytını denetler. Paket hatalıysa ölçümü yeniden ister.
4. Değerleri 10'a bölerek `field1`–`field5` alanlarına yayımlar.

USB seri porta yazılan her bayt LoRa üzerinden istasyona iletilir. Bu sayede istasyon elle de uyandırılabilir.

**Kurulum:**

1. Arduino IDE → *Boards Manager* ile **esp32** (Espressif) çekirdeğini, *Library Manager* ile **PubSubClient** kütüphanesini kurun.
2. `secrets.example.h` dosyasını `secrets.h` adıyla kopyalayın ve doldurun. Gereken MQTT bilgileri ThingSpeak'te *Devices → MQTT → Add a new device* ile oluşturulur; cihaza kanalınızı yayımlama yetkisi verin.
3. Bağlantılar: E220 TXD → GPIO16, E220 RXD → GPIO17. Modülün normal (iletim) kipinde çalışması için M0 ve M1 LOW olmalıdır. Kodda `M0_PIN` 4, `M1_PIN` 5 olarak tanımlı, ancak `setup()` içinde GPIO5 ve GPIO18 LOW yapılıyor. Bu yüzden M0 ve M1'i en basit yoldan doğrudan GND'ye bağlayabilirsiniz.

> `secrets.h` dosyası `.gitignore`'da listelenmiştir. Kimlik bilgilerinizi depoya göndermeyin.

## Bilinen sınırlılıklar

Ölçek uyuşmazlığı, kapalı uyku kipi ve LoRa kanal ayarı için bkz. ana [README](../README.md#bilinen-sınırlılıklar).

## Lisans

Proje kodu [MIT](../LICENSES/MIT.txt) lisanslıdır. STM32CubeMX'in ürettiği dosyalar ve ST dosyaları kendi başlıklarında belirtilen koşullara tabidir.
