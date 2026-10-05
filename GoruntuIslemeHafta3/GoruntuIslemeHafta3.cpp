#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// Hazir CLAHE fonksiyonu kullanmadan CLAHE uygular.
// Girdi: 8 bit, tek kanalli goruntu.
cv::Mat manuelCLAHE(
    const cv::Mat& kaynak,
    double clipLimit,
    int bolgeSayisiX,
    int bolgeSayisiY)
{
    CV_Assert(!kaynak.empty());
    CV_Assert(kaynak.type() == CV_8UC1);
    CV_Assert(bolgeSayisiX > 0 && bolgeSayisiY > 0);

    // 1. Goruntuyu bolgelere esit bolunebilecek boyuta getir.
    // OpenCV'nin kullandigi yansitmali kenar yontemini uygular.
    cv::Mat genisletilmis;

    if (kaynak.cols % bolgeSayisiX == 0 &&
        kaynak.rows % bolgeSayisiY == 0)
    {
        genisletilmis = kaynak;
    }
    else
    {
        int altEkleme =
            bolgeSayisiY - (kaynak.rows % bolgeSayisiY);

        int sagEkleme =
            bolgeSayisiX - (kaynak.cols % bolgeSayisiX);

        cv::copyMakeBorder(
            kaynak,
            genisletilmis,
            0, altEkleme,
            0, sagEkleme,
            cv::BORDER_REFLECT_101
        );
    }

    int bolgeGenisligi = genisletilmis.cols / bolgeSayisiX;
    int bolgeYuksekligi = genisletilmis.rows / bolgeSayisiY;

    int bolgePikselSayisi = bolgeGenisligi * bolgeYuksekligi;

    // clipLimit parametresini histogram sayim sinirina cevir.
    // clipLimit <= 0 olursa kirpma uygulanmaz.
    int kirpmaSiniri = 0;

    if (clipLimit > 0.0)
    {
        kirpmaSiniri = std::max(
            1,
            static_cast<int>(
                clipLimit * bolgePikselSayisi / 256.0
                )
        );
    }

    float olcek = 255.0f / bolgePikselSayisi;

    // Her bolge icin 256 elemanli donusum tablosu.
    std::vector<std::vector<unsigned char>> tablolar(
        bolgeSayisiX * bolgeSayisiY,
        std::vector<unsigned char>(256)
    );

    // 2. Her bolgenin histogramini hesapla.
    for (int by = 0; by < bolgeSayisiY; by++)
    {
        for (int bx = 0; bx < bolgeSayisiX; bx++)
        {
            int histogram[256] = { 0 };

            int baslangicX = bx * bolgeGenisligi;
            int baslangicY = by * bolgeYuksekligi;

            for (int y = baslangicY;
                y < baslangicY + bolgeYuksekligi;
                y++)
            {
                for (int x = baslangicX;
                    x < baslangicX + bolgeGenisligi;
                    x++)
                {
                    unsigned char deger =
                        genisletilmis.at<unsigned char>(y, x);

                    histogram[deger]++;
                }
            }

            // 3. Siniri asan histogram sayimlarini kirp.
            if (kirpmaSiniri > 0)
            {
                int fazlalik = 0;

                for (int i = 0; i < 256; i++)
                {
                    if (histogram[i] > kirpmaSiniri)
                    {
                        fazlalik += histogram[i] - kirpmaSiniri;
                        histogram[i] = kirpmaSiniri;
                    }
                }

                // Kirpilan sayimlari histogram kutularina dagit.
                int herKutuya = fazlalik / 256;
                int kalan = fazlalik % 256;

                for (int i = 0; i < 256; i++)
                {
                    histogram[i] += herKutuya;
                }

                // Kalan sayimlari aralikli kutulara dagit.
                if (kalan > 0)
                {
                    int adim = std::max(256 / kalan, 1);

                    for (int i = 0; i < 256 && kalan > 0;
                        i += adim)
                    {
                        histogram[i]++;
                        kalan--;
                    }
                }
            }

            // 4. Kumulatif histogramdan donusum tablosu olustur.
            int kumulatif = 0;
            int bolgeIndeksi = by * bolgeSayisiX + bx;

            for (int i = 0; i < 256; i++)
            {
                kumulatif += histogram[i];

                // kumulatif / bolgePikselSayisi = CDF
                // CDF * 255 = yeni parlaklik
                tablolar[bolgeIndeksi][i] =
                    cv::saturate_cast<unsigned char>(
                        kumulatif * olcek
                    );
            }
        }
    }

    // 5. Komsu bolgelerin tablolarini bilineer olarak harmanla.
    cv::Mat sonuc(kaynak.size(), CV_8UC1);

    float tersGenislik = 1.0f / bolgeGenisligi;
    float tersYukseklik = 1.0f / bolgeYuksekligi;

    for (int y = 0; y < kaynak.rows; y++)
    {
        float bolgeY = y * tersYukseklik - 0.5f;

        int ust = static_cast<int>(std::floor(bolgeY));
        int alt = ust + 1;

        float altAgirlik = bolgeY - ust;
        float ustAgirlik = 1.0f - altAgirlik;

        ust = std::max(ust, 0);
        alt = std::min(alt, bolgeSayisiY - 1);

        for (int x = 0; x < kaynak.cols; x++)
        {
            float bolgeX = x * tersGenislik - 0.5f;

            int sol = static_cast<int>(std::floor(bolgeX));
            int sag = sol + 1;

            float sagAgirlik = bolgeX - sol;
            float solAgirlik = 1.0f - sagAgirlik;

            sol = std::max(sol, 0);
            sag = std::min(sag, bolgeSayisiX - 1);

            unsigned char deger =
                kaynak.at<unsigned char>(y, x);

            float ustSol =
                tablolar[ust * bolgeSayisiX + sol][deger];

            float ustSag =
                tablolar[ust * bolgeSayisiX + sag][deger];

            float altSol =
                tablolar[alt * bolgeSayisiX + sol][deger];

            float altSag =
                tablolar[alt * bolgeSayisiX + sag][deger];

            float yeniDeger =
                (ustSol * solAgirlik + ustSag * sagAgirlik)
                * ustAgirlik
                +
                (altSol * solAgirlik + altSag * sagAgirlik)
                * altAgirlik;

            sonuc.at<unsigned char>(y, x) =
                cv::saturate_cast<unsigned char>(yeniDeger);
        }
    }

    return sonuc;
}

int main()
{
    cv::Mat goruntu =
        cv::imread("yol.jpeg", cv::IMREAD_GRAYSCALE);

    if (goruntu.empty())
    {
        std::cerr << "yol.jpeg acilamadi.\n";
        return 1;
    }

    double clipLimit = 2.0;
    int bolgeSayisiX = 8;
    int bolgeSayisiY = 8;

    // A. OpenCV'nin hazir CLAHE fonksiyonu
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(
        clipLimit,
        cv::Size(bolgeSayisiX, bolgeSayisiY)
    );

    cv::Mat hazirSonuc;
    clahe->apply(goruntu, hazirSonuc);

    // B. Kendi yazdigimiz CLAHE
    cv::Mat manuelSonuc = manuelCLAHE(
        goruntu,
        clipLimit,
        bolgeSayisiX,
        bolgeSayisiY
    );

    // C. Iki sonucu piksel piksel karsilastir
    cv::Mat fark(goruntu.size(), CV_8UC1);

    long long toplamFark = 0;
    long long farkliPiksel = 0;
    int enBuyukFark = 0;

    for (int y = 0; y < goruntu.rows; y++)
    {
        for (int x = 0; x < goruntu.cols; x++)
        {
            int hazir = hazirSonuc.at<unsigned char>(y, x);
            int manuel = manuelSonuc.at<unsigned char>(y, x);

            int pikselFarki = std::abs(hazir - manuel);

            fark.at<unsigned char>(y, x) =
                static_cast<unsigned char>(pikselFarki);

            toplamFark += pikselFarki;
            enBuyukFark = std::max(enBuyukFark, pikselFarki);

            if (pikselFarki != 0)
            {
                farkliPiksel++;
            }
        }
    }

    long long toplamPiksel =
        static_cast<long long>(goruntu.rows) * goruntu.cols;

    double ortalamaFark =
        static_cast<double>(toplamFark) / toplamPiksel;

    std::cout << "Ortalama mutlak fark: "
        << ortalamaFark << "\n";

    std::cout << "En buyuk piksel farki: "
        << enBuyukFark << "\n";

    std::cout << "Farkli piksel sayisi: "
        << farkliPiksel << " / " << toplamPiksel << "\n";

    // D. Sonuclari kaydet
    bool hazirKaydedildi =
        cv::imwrite("clahe_opencv.png", hazirSonuc);

    bool manuelKaydedildi =
        cv::imwrite("clahe_manuel.png", manuelSonuc);

    bool farkKaydedildi =
        cv::imwrite("clahe_fark.png", fark);

    if (!hazirKaydedildi ||
        !manuelKaydedildi ||
        !farkKaydedildi)
    {
        std::cerr << "Cikti dosyalari kaydedilemedi.\n";
        return 1;
    }

    // E. Ekranda goster
    cv::namedWindow("Orijinal", cv::WINDOW_NORMAL);
    cv::namedWindow("OpenCV CLAHE", cv::WINDOW_NORMAL);
    cv::namedWindow("Manuel CLAHE", cv::WINDOW_NORMAL);

    cv::imshow("Orijinal", goruntu);
    cv::imshow("OpenCV CLAHE", hazirSonuc);
    cv::imshow("Manuel CLAHE", manuelSonuc);

    cv::waitKey(0);
    return 0;
}