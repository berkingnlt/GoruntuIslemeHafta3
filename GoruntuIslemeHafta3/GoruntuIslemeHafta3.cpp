#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>

int main()
{
    // 1. Gurultulu goruntuyu gri tonlamali ac
    cv::Mat orijinal =
        cv::imread("yol.jpeg", cv::IMREAD_GRAYSCALE);

    if (orijinal.empty())
    {
        std::cerr << "yol.jpeg acilamadi.\n";
        return 1;
    }

    // Negatif gurultu degerlerini korumak icin 16 bit kullan
    cv::Mat orijinal16;
    orijinal.convertTo(orijinal16, CV_16SC1);

    // Ortalama 0, standart sapma 25 olan Gauss gurultusu
    cv::Mat gurultu(orijinal.size(), CV_16SC1);
    cv::randn(gurultu, 0, 25);

    cv::Mat gurultulu16 = orijinal16 + gurultu;

    // Degerleri 0-255 araligina sinirlayarak 8 bite donustur
    cv::Mat goruntu;
    gurultulu16.convertTo(goruntu, CV_8UC1);

    if (!cv::imwrite("gurultulu.png", goruntu))
    {
        std::cerr << "Gurultulu resim kaydedilemedi.\n";
        return 1;
    }

    if (goruntu.empty())
    {
        std::cerr << "gurultulu.jpeg acilamadi.\n";
        return 1;
    }

    // 2. 3x3 ortalama filtresi
    const double cekirdek[3][3] =
    {
        {1.0 / 9, 1.0 / 9, 1.0 / 9},
        {1.0 / 9, 1.0 / 9, 1.0 / 9},
        {1.0 / 9, 1.0 / 9, 1.0 / 9}
    };

    // Sonucu ayri goruntude tut.
    // Hesaplamalarda daima orijinal pikselleri kullan.
    cv::Mat sonuc(goruntu.size(), CV_8UC1);

    // 3. Filtreyi tum goruntu uzerinde gezdir
    for (int y = 0; y < goruntu.rows; y++)
    {
        for (int x = 0; x < goruntu.cols; x++)
        {
            double toplam = 0.0;

            // Her pikselin 3x3 komsulugunu ziyaret et
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    int komsuY = y + dy;
                    int komsuX = x + dx;

                    // Sinir disinda en yakin kenar pikselini kullan
                    komsuY = std::max(
                        0, std::min(komsuY, goruntu.rows - 1)
                    );

                    komsuX = std::max(
                        0, std::min(komsuX, goruntu.cols - 1)
                    );

                    unsigned char piksel =
                        goruntu.at<unsigned char>(komsuY, komsuX);

                    // Konvolusyon: cekirdegi ters indeksle
                    toplam += piksel * cekirdek[1 - dy][1 - dx];
                }
            }

            // Ortalamayi yuvarlayip sonuc pikseline yaz
            sonuc.at<unsigned char>(y, x) =
                static_cast<unsigned char>(toplam + 0.5);
        }
    }

    // 4. Sonucu kaydet
    if (!cv::imwrite("ortalama_3x3.png", sonuc))
    {
        std::cerr << "Sonuc kaydedilemedi.\n";
        return 1;
    }

    std::cout << "Sonuc ortalama_3x3.png olarak kaydedildi.\n";

    // 5. Orijinal ve filtrelenmis goruntuyu goster
    cv::namedWindow("Gurultulu Goruntu", cv::WINDOW_NORMAL);
    cv::namedWindow("3x3 Ortalama Filtresi", cv::WINDOW_NORMAL);

    cv::imshow("Gurultulu Goruntu", goruntu);
    cv::imshow("3x3 Ortalama Filtresi", sonuc);

    cv::waitKey(0);
    return 0;
}