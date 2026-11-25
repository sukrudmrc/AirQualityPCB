# Test verileri

Bu klasörde makalede sunulan sensör birim testlerinin ham kayıtları bulunur. Dosyalar alındıkları biçimde korunmuştur. Tek istisna SDS011 kaydıdır: bu kayıt bir Excel dosyasından CSV'ye dönüştürülmüştür, değerler değiştirilmemiştir.

Makaledeki şekil ve istatistikler [`../betikler/sekil_uret.py`](../betikler/sekil_uret.py) ile bu dosyalardan üretilir.

| Dosya | Sensör | Satır | İçerik |
|---|---|---:|---|
| `sht31_oturum1.csv` | SHT31 | 3983 | Zaman damgası, sıcaklık (°C), bağıl nem (%) |
| `sht31_oturum2.csv` | SHT31 | 3056 | Aynı biçim |
| `sht31_oturum3.csv` | SHT31 | 1000 | Aynı sütunlar. Değerler `Temperature: 26.75 , Humidity: 65.99` biçiminde metin olarak yazılmıştır |
| `mics5524_co.csv` | MiCS-5524 | 77 943 | Örnek numarası ve sensör çıkış gerilimi (V) |
| `sds011_pm.csv` | SDS011 | 100 | PM2.5 ve PM10 (µg/m³), ardışık örnekler |

## Ölçüm düzeni

- **SHT31:** Sensör bir ESP32 geliştirme kartıyla okundu. Değerler seri port üzerinden [`../betikler/seri_kayit.py`](../betikler/seri_kayit.py) ile zaman damgası eklenerek kaydedildi (18 Ocak 2025). Her oturumda sensör sıcak bir ortamdan daha soğuk bir ortama taşınarak ortam değişikliğine verdiği yanıt gözlendi.
- **MiCS-5524:** Sensörün analog çıkışı ADC ile okundu ve uzun süre izlendi. Makalede örnekleme periyodu 1 s (zamanlayıcı kesmesi) olarak alınmıştır; buna göre kayıt yaklaşık 27 saati kapsar. Seri port bozulmasından kaynaklanan 5 V üstündeki tek örnek, şekil betiğinde ayıklanır.
- **SDS011:** Sensör sürekli ölçüm kipinde çalıştırıldı ve ardışık 100 çerçeve kaydedildi.

Bu kayıtlar kalibrasyon amaçlı değildir. Sensörlerin çalıştığını ve ortam değişikliğine tepki verdiğini göstermek için alınmıştır.

## Lisans

[CC BY 4.0](../LICENSES/CC-BY-4.0.txt). Kullanırken lütfen [atıf](../README.md#atıf) yapın.
