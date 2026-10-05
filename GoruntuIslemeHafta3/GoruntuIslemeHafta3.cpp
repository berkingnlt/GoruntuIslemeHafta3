#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <cmath>

int main()
{
    // Proje klasorundeki dosya yollari
    std::string girisYolu = "yol.jpeg";
    std::string cikisYolu = "esitlenmis.png";
    std::string tabloYolu = "histogram_cdf.csv";

    // 1. Goruntuyu tek kanalli (gri tonlamali) ac
    cv::Mat goruntu = cv::imread(girisYolu, cv::IMREAD_GRAYSCALE);

    if (goruntu.empty())
    {
        std::cerr << "Goruntu acilamadi: " << girisYolu << "\n";
        return 1;
    }

    // 2. 256 kutulu histogrami elle hesapla
    long long histogram[256] = { 0 };

    for (int y = 0; y < goruntu.rows; y++)
    {
        for (int x = 0; x < goruntu.cols; x++)
        {
            unsigned char piksel = goruntu.at<unsigned char>(y, x);
            histogram[piksel]++;
        }
    }

    // 3. Kumulatif sayim ve CDF hesapla
    long long toplamPiksel =
        static_cast<long long>(goruntu.rows) * goruntu.cols;

    long long kumulatifToplam = 0;
    long long kumulatifSayim[256] = { 0 };
    double cdf[256] = { 0.0 };

    for (int i = 0; i < 256; i++)
    {
        kumulatifToplam += histogram[i];
        kumulatifSayim[i] = kumulatifToplam;

        cdf[i] = static_cast<double>(kumulatifToplam)
            / toplamPiksel;
    }

    // 4. CDF ile yeni parlaklik degerlerini hesapla
    unsigned char yeniDeger[256] = { 0 };

    for (int i = 0; i < 256; i++)
    {
        yeniDeger[i] =
            static_cast<unsigned char>(std::round(255.0 * cdf[i]));
    }

    // 5. Esitlemeyi her piksele elle uygula
    cv::Mat sonuc(goruntu.rows, goruntu.cols, CV_8UC1);

    for (int y = 0; y < goruntu.rows; y++)
    {
        for (int x = 0; x < goruntu.cols; x++)
        {
            unsigned char eskiDeger =
                goruntu.at<unsigned char>(y, x);

            sonuc.at<unsigned char>(y, x) = yeniDeger[eskiDeger];
        }
    }

    // 6. Cikti resmini kaydet
    if (!cv::imwrite(cikisYolu, sonuc))
    {
        std::cerr << "Sonuc resmi kaydedilemedi.\n";
        return 1;
    }

    // 7. Histogram ve CDF tablosunu kaydet
    std::ofstream tablo(tabloYolu);

    if (!tablo)
    {
        std::cerr << "Tablo dosyasi olusturulamadi.\n";
        return 1;
    }

    tablo << "Parlaklik,Histogram,KumulatifSayim,CDF,YeniDeger\n";

    for (int i = 0; i < 256; i++)
    {
        tablo << i << ","
            << histogram[i] << ","
            << kumulatifSayim[i] << ","
            << cdf[i] << ","
            << static_cast<int>(yeniDeger[i]) << "\n";
    }

    tablo.close();

    std::cout << "Toplam piksel: " << toplamPiksel << "\n";
    std::cout << "Histogram toplami: " << kumulatifToplam << "\n";
    std::cout << "CDF[255]: " << cdf[255] << "\n";
    std::cout << "Resim kaydedildi: " << cikisYolu << "\n";
    std::cout << "Tablo kaydedildi: " << tabloYolu << "\n";

    // 8. Orijinal ve sonuc goruntulerini goster
    cv::namedWindow("Orijinal", cv::WINDOW_NORMAL);
    cv::namedWindow("Esitlenmis", cv::WINDOW_NORMAL);

    cv::imshow("Orijinal", goruntu);
    cv::imshow("Esitlenmis", sonuc);

    cv::waitKey(0);
    return 0;
}