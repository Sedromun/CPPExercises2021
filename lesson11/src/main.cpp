#include <filesystem>
#include <iostream>
#include <libutils/rasserts.h>

#include "parseSymbols.h"
#include "hog.h"

#include <opencv2/imgproc.hpp>


cv::Scalar randColor() {
    return cv::Scalar(128 + rand() % 128, 128 + rand() % 128, 128 + rand() % 128); // можно было бы брать по модулю 255, но так цвета будут светлее и контрастнее
}


cv::Mat drawContours(int rows, int cols, std::vector<std::vector<cv::Point>> contoursPoints) {

    // TODO 06 реализуйте функцию которая покажет вам как выглядят найденные контура:

    // создаем пустую черную картинку
    cv::Mat blackImage(rows, cols, CV_8UC3, cv::Scalar(0, 0, 0));
    // теперь мы на ней хотим нарисовать контуры
    cv::Mat imageWithContoursPoints = blackImage.clone();
    for (int contourI = 0; contourI < contoursPoints.size(); ++contourI) {
        // сейчас мы смотрим на контур номер contourI

        cv::Scalar contourColor = randColor(); // выберем для него случайный цвет
        std::vector<cv::Point> points = contoursPoints[contourI]; // TODO 06 вытащите вектор из точек-пикселей соответствующих текущему контуру который мы хотим нарисовать
        for (int i = 0; i < points.size(); i++) { // TODO 06 пробегите по всем точкам-пикселям этого контура
            cv::Point point = points[i]; // TODO 06 и взяв очередную точку-пиксель - нарисуйте выбранный цвет в этом пикселе картинки:
            imageWithContoursPoints.at<cv::Vec3b>(point.y, point.x) = cv::Vec3b(contourColor[0], contourColor[1], contourColor[2]);
        }

    }

    return imageWithContoursPoints;
}

bool comp(std::vector<cv::Point> a, std::vector<cv::Point> b)
{
    cv::Rect boxA = cv::boundingRect(a);
    cv::Rect boxB = cv::boundingRect(b);
    return (boxA.br().x < boxB.br().x);
}

void test(std::string name, std::string k) {
    std::cout << "Processing " << name << "/" << k << "..." << std::endl;

    std::string full_path = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson11\\data\\" + name + "\\" + k + ".png";

    // создаем папочки в которые будем сохранять картинки с промежуточными результатами
    std::filesystem::create_directory("C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson11\\resultsData\\" + name);
    std::string out_path = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson11\\resultsData\\" + name + "\\" + k;
    std::filesystem::create_directory(out_path);

    // считываем оригинальную исходную картинку
    cv::Mat original = cv::imread(full_path);
    rassert(!original.empty(), 238982391080010);
    rassert(original.type() == CV_8UC3, 23823947238900020);

    // сохраняем ее сразу для удобства
    cv::imwrite(out_path + "/00_original.jpg", original);

    // преобразуем в черно-белый цвет и тоже сразу сохраняем
    cv::Mat img;
    cv::cvtColor(original, img, cv::COLOR_BGR2GRAY);
    cv::imwrite(out_path + "/01_grey.jpg", img);

    // TODO 01 выполните бинарный трешолдинг картинки, прочитайте документацию по функции cv::threshold и выберите значения аргументов
    cv::Mat binary;
    cv::threshold(img, binary, 170, 255, cv::THRESH_BINARY_INV);
    cv::imwrite(out_path + "/02_binary_thresholding.jpg", binary);

    // TODO 02 выполните адаптивный бинарный трешолдинг картинки, прочитайте документацию по cv::adaptiveThreshold
    cv::adaptiveThreshold(img, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 21, 30);
    cv::imwrite(out_path + "/03_adaptive_thresholding.jpg", binary);

    // TODO 03 чтобы буквы не разваливались на кусочки - морфологическое расширение (эрозия)
    cv::Mat binary_eroded;
    cv::erode(binary, binary_eroded, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4)));
    cv::imwrite(out_path + "/04_erode.jpg", binary_eroded);

    // TODO 03 заодно давайте посмотрим что делает морфологическое сужение (диляция)
    cv::Mat binary_dilated;
    cv::dilate(binary, binary_dilated, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4)));
    cv::imwrite(out_path + "/05_dilate.jpg", binary_dilated);

    // TODO 04 дальше работаем с картинкой после морфологичесокго рашсирения или морфологического сжатия - на ваш выбор, подумайте и посмотрите на картинки
    binary = binary_dilated;

    // TODO 05
    std::vector<std::vector<cv::Point>> contoursPoints; // по сути это вектор, где каждый элемент - это одна связная компонента-контур,
                                                        // а что такое компонента-контур? это вектор из точек (из пикселей)
    cv::findContours(binary, contoursPoints, cv::RETR_LIST, cv::CHAIN_APPROX_TC89_KCOS ); // TODO подумайте, какие нужны два последних параметра? прочитайте документацию, после реализации отрисовки контура - поиграйте с этими параметрами чтобы посмотреть как меняется результат
    std::cout << "Contours: " << contoursPoints.size() << std::endl;
    cv::Mat imageWithContoursPoints = drawContours(img.rows, img.cols, contoursPoints); // TODO 06 реализуйте функцию которая покажет вам как выглядят найденные контура
    cv::imwrite(out_path + "/06_contours_points.jpg", imageWithContoursPoints);

    std::vector<std::vector<cv::Point>> contoursPoints2;
    cv::findContours(binary, contoursPoints2, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);
    // TODO:
    // Обратите внимание на кромку картинки - она всё победила, т.к. черное - это ноль - пустота, а белое - это 255 - сам объект интереса
    // как перевернуть ситуацию чтобы периметр не был засчитан как контур?
    // когда подумаете - замрите! и прежде чем кодить:
    // Посмотрите в документации у функций cv::threshold и cv::adaptiveThreshold
    // про некоего cv::THRESH_BINARY_INV, чем он отличается от cv::THRESH_BINARY?
    // Посмотрите как изменились все картинки.
    std::cout << "Contours2: " << contoursPoints2.size() << std::endl;
    cv::Mat imageWithContoursPoints2 = drawContours(img.rows, img.cols, contoursPoints2);
    cv::imwrite(out_path + "/07_contours_points2.jpg", imageWithContoursPoints2);

    // TODO 06 наконец давайте посмотрим какие буковки нашлись - обрамим их прямоугольниками
    cv::Mat imgWithBoxes = original.clone();
    std::vector<int> sizes;
    std::vector<cv::Point> centers;
    for (int contourI = 0; contourI < contoursPoints2.size(); ++contourI) {
        std::vector<cv::Point> points = contoursPoints2[contourI]; // перем очередной контуr
        cv::Rect box = cv::boundingRect(points); // строим прямоугольник по всем пикселям контура (bounding box = бокс ограничивающий объект)
        cv::Scalar blackColor(0, 0, 0);
        sizes.push_back(box.height);
        centers.push_back(cv::Point(box.width/2, box.height/2));
        // TODO прочитайте документацию cv::rectangle чтобы нарисовать прямоугольник box с толщиной 2 и черным цветом (обратите внимание какие есть поля у box)
        cv::rectangle(imgWithBoxes, box, blackColor, 2, cv::LINE_4);
    }
    cv::imwrite(out_path + "/08_boxes.jpg", imgWithBoxes); // TODO если вдруг у вас в картинке странный результат
    // например если нет прямоугольников - посмотрите в верхний левый пиксель - белый ли он?
    sort(sizes.begin(), sizes.end());
    int median = sizes[sizes.size()/2];
    int str = 0;
    std::vector<std::vector<std::vector<cv::Point>>> stroki(centers.size());
    for(int i = 1; i < centers.size(); i++)
    {
        if(centers[i].y - centers[i-1].y > median*2/3)
            str++;
        stroki[str].push_back(contoursPoints2[i]);
    }


    for(int strI = 0; strI < stroki.size(); strI++){
        if(stroki[strI].size() == 0)
            continue;
        std::vector<std::vector<cv::Point>> curString = stroki[strI];
        sort(curString.begin(), curString.end(), comp);
        for(int i = 0; i < curString.size(); i++){
            cv::Rect box = cv::boundingRect(curString[i]);
            cv::Mat leftROI = original(box);
            HoG hog = buildHoG(leftROI);
            for (char letter = 'a'; letter <= 'z'; ++letter)
            {
                std::string LETTER_DIR_PATH = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson11\\generatedData\\";
                std::string letterDir = LETTER_DIR_PATH + letter + std::to_string(1) + ".png";
                cv::Mat perfect = cv::imread(letterDir);
                rassert(!perfect.empty(), 238982391080111);
                HoG perfectHoG = buildHoG(perfect);
                if(distance(hog, perfectHoG) < 5)
                    std::cout << letter << " ";
            }

        }
    }


}

//void finalExperiment(std::string name, std::string k) {
//    // TODO 100:
//    // 1) вытащите результат которым вы довольны в функцию splitSymbols в parseSymbols.h/parseSymbols.cpp
//    //    эта функция должна находить контуры букв и извлекать кусочки картинок в вектор
//    // 2) классифицируйте каждую из вытащенных букв (результатом из прошлого задания) и выведите полученный текст в консоль
//    std::cout << "Processing " << name << "/" << k << "..." << std::endl;
//
//    std::string full_path = "H:\\CLionProjects\\CPPExercises2021\\lesson11\\data\\" + name + "\\" + k + ".png";
//
//    // создаем папочки в которые будем сохранять картинки с промежуточными результатами
//    std::filesystem::create_directory("H:\\CLionProjects\\CPPExercises2021\\lesson11\\resultsData\\" + name);
//    std::string out_path = "H:\\CLionProjects\\CPPExercises2021\\lesson11\\resultsData\\" + name + "\\" + k;
//    std::filesystem::create_directory(out_path);
//
//    // считываем оригинальную исходную картинку
//    cv::Mat original = cv::imread(full_path);
//    rassert(!original.empty(), 238982391080010);
//    rassert(original.type() == CV_8UC3, 23823947238900020);
//
//    std::vector<cv::Mat> allSymbols = splitSymbols(original);
//
//}


int main() {
    try {
        test("alphabet", "3_gradient");

//         TODO 50: обязательно получите результат на других картинках - прямо в цикле все их обработайте:
        std::vector<std::string> names;
        names.push_back("alphabet");
        names.push_back("line");
        names.push_back("text");
        for (int i = 0; i < names.size(); ++i) {
            for (int j = 1; j <= 5; ++j) {
                test(names[i], std::to_string(j));
            }
        }
//
//        test("alphabet", "3_gradient");

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}

