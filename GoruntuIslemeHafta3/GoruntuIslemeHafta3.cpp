#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <iostream>
#include <algorithm>
#include <string>

// 25 elemani kucukten buyuge elle sirala
void sirala(int degerler[], int adet)
{
    // Ekleme siralamasi (insertion sort)
    for (int i = 1; i < adet; i++)
    {
        int mevcut = degerler[i];
        int j = i - 1;

        while (j >= 0 && degerler[j] > mevcut)
        {
            degerler[j + 1] = degerler[j];
            j--;
        }

        degerler[j + 1] = mevcut;
    }
}

// Hazir medyan filtre kullanmadan 5x5 filtre uygula
cv::Mat manuelMedyan5x5(const cv::Mat& kaynak)
{
    CV_Assert(!kaynak.empty());
    CV_Assert(kaynak.type() == CV_8UC1);

    cv::Mat sonuc(kaynak.size(), CV_8UC1);

    for (int y = 0; y < kaynak.rows; y++)
    {
        for (int x = 0; x < kaynak.cols; x++)
        {
            int degerler[25];
            int indeks = 0;

            // 5x5 komsuluktaki 25 pikseli topla
            for (int dy = -2; dy <= 2; dy++)
            {
                for (int dx = -2; dx <= 2; dx++)
                {
                    int komsuY = y + dy;
                    int komsuX = x + dx;

                    // Sinirlarda en yakin kenar pikselini kullan
                    komsuY = std::max(
                        0, std::min(komsuY, kaynak.rows - 1)
                    );

                    komsuX = std::max(
                        0, std::min(komsuX, kaynak.cols - 1)
                    );

                    degerler[indeks] =
                        kaynak.at<unsigned char>(komsuY, komsuX);

                    indeks++;
                }
            }

            sirala(degerler, 25);

            // Dizide indeksler 0'dan baslar:
            // 13. elemanin indeksi 12'dir.
            sonuc.at<unsigned char>(y, x) =
                static_cast<unsigned char>(degerler[12]);
        }
    }

    return sonuc;
}

int main()
{
    cv::Mat goruntu;

    // Salt-and-Pepper goruntusu varsa onu kullan
    cv::String gurultuluYol = "salt_pepper.png";

    if (cv::utils::fs::exists(gurultuluYol))
    {
        goruntu = cv::imread(
            gurultuluYol,
            cv::IMREAD_GRAYSCALE
        );
    }
    else
    {
        // Yoksa yol.jpeg uzerinden test goruntusu olustur
        cv::Mat orijinal =
            cv::imread("yol.jpeg", cv::IMREAD_GRAYSCALE);

        if (orijinal.empty())
        {
            std::cerr << "yol.jpeg acilamadi.\n";
            return 1;
        }

        goruntu = orijinal.clone();

        // Sabit tohum: her calistirmada ayni gurultu
        cv::RNG rastgele(42);

        for (int y = 0; y < goruntu.rows; y++)
        {
            for (int x = 0; x < goruntu.cols; x++)
            {
                int sans = rastgele.uniform(0, 100);

                if (sans < 5)
                {
                    // %5 olasilikla siyah
                    goruntu.at<unsigned char>(y, x) = 0;
                }
                else if (sans < 10)
                {
                    // %5 olasilikla beyaz
                    goruntu.at<unsigned char>(y, x) = 255;
                }
            }
        }

        if (!cv::imwrite("salt_pepper.png", goruntu))
        {
            std::cerr << "Gurultulu resim kaydedilemedi.\n";
            return 1;
        }
    }

    if (goruntu.empty())
    {
        std::cerr << "Salt-and-Pepper goruntusu acilamadi.\n";
        return 1;
    }

    // 1. OpenCV'nin hazir 5x5 medyan filtresi
    cv::Mat hazirSonuc;
    cv::medianBlur(goruntu, hazirSonuc, 5);

    // 2. Kendi yazdigimiz 5x5 medyan filtresi
    cv::Mat manuelSonuc = manuelMedyan5x5(goruntu);

    // 3. Sonuclari piksel piksel karsilastir
    cv::Mat fark(goruntu.size(), CV_8UC1);

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

            if (pikselFarki != 0)
            {
                farkliPiksel++;
            }

            enBuyukFark = std::max(enBuyukFark, pikselFarki);
        }
    }

    std::cout << "Farkli piksel sayisi: "
        << farkliPiksel << "\n";

    std::cout << "En buyuk piksel farki: "
        << enBuyukFark << "\n";

    // 4. Ciktilari kaydet
    bool kayit1 = cv::imwrite(
        "medyan_opencv_5x5.png", hazirSonuc
    );

    bool kayit2 = cv::imwrite(
        "medyan_manuel_5x5.png", manuelSonuc
    );

    bool kayit3 = cv::imwrite(
        "medyan_fark.png", fark
    );

    if (!kayit1 || !kayit2 || !kayit3)
    {
        std::cerr << "Ciktilar kaydedilemedi.\n";
        return 1;
    }

    // 5. Goruntuleri goster
    cv::namedWindow("Salt-and-Pepper", cv::WINDOW_NORMAL);
    cv::namedWindow("OpenCV Medyan 5x5", cv::WINDOW_NORMAL);
    cv::namedWindow("Manuel Medyan 5x5", cv::WINDOW_NORMAL);

    cv::imshow("Salt-and-Pepper", goruntu);
    cv::imshow("OpenCV Medyan 5x5", hazirSonuc);
    cv::imshow("Manuel Medyan 5x5", manuelSonuc);

    cv::waitKey(0);
    return 0;
}