#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <chrono>
#include <iostream>
#include <filesystem>
#include <memory>

#include <libutils/rasserts.h>

bool isPixelEmpty(cv::Vec3b color) {
    // - верните true если переданный цвет - полностью черный (такие пиксели мы считаем пустыми)
    // - иначе верните false
    if(color[0] + color[1] + color[2] == 0)
        return true;
    else
        return false;
}

void run(std::string caseName) {
    std::string path = "H:\\CLionProjects\\CPPExercises2021\\lesson16\\data\\";
    cv::Mat img0 = cv::imread( path + caseName + "/0.png");
    cv::Mat img1 = cv::imread(path + caseName + "/1.png");
    rassert(!img0.empty(), 324789374290018);
    rassert(!img1.empty(), 378957298420019);

    int downscale = 1; // уменьшим картинку в два раза столько раз сколько указано в этой переменной (итоговое уменьшение в 2^downscale раз)
    for (int i = 0; i < downscale; ++i) {
        cv::pyrDown(img0, img0); // уменьшаем картинку в два раза (по каждой из осей)
        cv::pyrDown(img1, img1); // уменьшаем картинку в два раза (по каждой из осей)
    }

    cv::Ptr<cv::FeatureDetector> detector = cv::SIFT::create();
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);

    // Детектируем и описываем дескрипторы ключевых точек
    std::vector<cv::KeyPoint> keypoints0, keypoints1;
    cv::Mat descriptors0, descriptors1;
    detector->detectAndCompute(img0, cv::noArray(), keypoints0, descriptors0);
    detector->detectAndCompute(img1, cv::noArray(), keypoints1, descriptors1);
    std::cout << "Keypoints initially: " << keypoints0.size() << ", " << keypoints1.size() << "..." << std::endl;

    // Сопоставляем ключевые точки (находя для каждой точки из первой картинки - две ближайшие, т.е. две самые похожие, с правой картинки)
    std::vector<std::vector<cv::DMatch>> matches01;
    matcher->knnMatch(descriptors0, descriptors1, matches01, 2);
    rassert(keypoints0.size() == matches01.size(), 349723894200068);

    // Фильтруем сопоставления от шума - используя K-ratio тест
    std::vector<cv::Point2f> points0;
    std::vector<cv::Point2f> points1;
    for (int i = 0; i < keypoints0.size(); ++i) {
        int fromKeyPoint0 = matches01[i][0].queryIdx;
        int toKeyPoint1Best = matches01[i][0].trainIdx;
        float distanceBest = matches01[i][0].distance;
        rassert(fromKeyPoint0 == i, 348723974920074);
        rassert(toKeyPoint1Best < keypoints1.size(), 347832974820076);

        int toKeyPoint1SecondBest = matches01[i][1].trainIdx;
        float distanceSecondBest = matches01[i][1].distance;
        rassert(toKeyPoint1SecondBest < keypoints1.size(), 3482047920081);
        rassert(distanceBest <= distanceSecondBest, 34782374920082);

        // простой K-ratio тест, но могло иметь смысл добавить left-right check
        if (distanceBest < 0.7 * distanceSecondBest) {
            points0.push_back(keypoints0[i].pt);
            points1.push_back(keypoints1[toKeyPoint1Best].pt);
        }
    }
    rassert(points0.size() == points1.size(), 234723947289089);
    std::cout << "Matches after K-ratio test: " << points0.size() << std::endl;

    // Находим матрицу преобразования второй картинки в систему координат первой картинки
    cv::Mat H10 = cv::findHomography(points1, points0, cv::RANSAC, 3.0);
    rassert(H10.size() == cv::Size(3, 3), 3482937842900059); // см. документацию https://docs.opencv.org/4.5.1/d9/d0c/group__calib3d.html#ga4abc2ece9fab9398f2e560d53c8c9780
                                                                             // "Note that whenever an H matrix cannot be estimated, an empty one will be returned."

    // создаем папку в которую будем сохранять результаты - lesson16/resultsData/ИМЯ_НАБОРА/
    std::string resultsDir = "H:\\CLionProjects\\CPPExercises2021\\lesson16\\resultsData\\";
    if (!std::filesystem::exists(resultsDir)) { // если папка еще не создана
        std::filesystem::create_directory(resultsDir); // то создаем ее
    }
    resultsDir += caseName + "/";
    if (!std::filesystem::exists(resultsDir)) { // если папка еще не создана
        std::filesystem::create_directory(resultsDir); // то создаем ее
    }

    cv::imwrite(resultsDir + "0img0.jpg", img0);
    cv::imwrite(resultsDir + "1img1.jpg", img1);

    // находим куда переходят углы второй картинки
    std::vector<cv::Point2f> corners1(4);
    corners1[0] = cv::Point(0, 0); // верхний левый
    corners1[1] = cv::Point(img1.cols, 0); // верхний правый
    corners1[2] = cv::Point(img1.cols, img1.rows); // нижний правый
    corners1[3] = cv::Point(0, img1.rows); // нижний левый
    std::vector<cv::Point2f> corners10(4);
    perspectiveTransform(corners1, corners10, H10);

    // находим какой ширины и высоты наша панорама (как минимум - разрешение первой картинки, но еще нужно учесть куда перешли углы второй картинки)
    int max_x = img0.cols;
    int max_y = img0.rows;
    for (int i = 0; i < 4; ++i) {
        max_x = std::max(max_x, (int) corners10[i].x);
        max_y = std::max(max_y, (int) corners10[i].y);
    }
    int pano_rows = max_y;
    int pano_cols = max_x;

    // преобразуем обе картинки в пространство координат нашей искомой панорамы
    cv::Mat pano0(pano_rows, pano_cols, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat pano1(pano_rows, pano_cols, CV_8UC3, cv::Scalar(0, 0, 0));
    rassert(img0.type() == CV_8UC3, 3423890003123093);
    rassert(img1.type() == CV_8UC3, 3423890003123094);
    // вторую картинку просто натягиваем в соответствии с ранее найденной матрицей Гомографии
    cv::warpPerspective(img1, pano1, H10, pano1.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    // первую картинку надо положить без каких-то смещений, т.е. используя единичную матрицу:
    cv::Mat identity_matrix = cv::Mat::eye(3, 3, CV_64FC1);
    cv::warpPerspective(img0, pano0, identity_matrix, pano1.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

    cv::imwrite(resultsDir + "2pano0.jpg", pano0);
    cv::imwrite(resultsDir + "3pano1.jpg", pano1);

    // давайте сделаем наивную панораму - наложим вторую картинку на первую:
    cv::Mat panoBothNaive = pano0.clone();
    cv::warpPerspective(img1, panoBothNaive, H10, panoBothNaive.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    cv::imwrite(resultsDir + "4panoBothNaive.jpg", panoBothNaive);

     // (т.е. эта функция позволит дальше понимать в этот пиксель наложилась исходная картинка или же там все еще тьма)

    cv::Mat panoDiff(pano_rows, pano_cols, CV_8UC3, cv::Scalar(0, 0, 0));
    // TODO 2 вам надо заполнить panoDiff картинку так чтобы было четко ясно где pano0 картинка (объявлена выше) и pano1 картинка отличаются сильно, а где - слабо:
    // сравните в этих двух картинках пиксели по одинаковым координатам (т.е. мы сверяем картинки) и покрасьте соответствующий пиксель panoDiff по этой логике:
    // - если оба пикселя пустые - проверяйте это через isPixelEmpty(color) (т.е. цвета черные) - результат тоже пусть черный
    // - если ровно один их пикселей пустой - результат пусть идеально белый
    // - иначе пусть результатом будет оттенок серого - пусть он тем светлее, чем больше разница между цветами пикселей
    // При этом сделайте так чтобы самый сильно отличающийся пиксель - всегда был идеально белым (255), т.е. выполните нормировку с учетом того какая максимальная разница яркости присутствует
    // Напоминание - вот так можно выставить цвет в пикселе:
    //  panoDiff.at<cv::Vec3b>(j, i) = cv::Vec3b(blueValue, greenValue, redValue);

    for(int i = 0; i < panoDiff.cols; i++){
        for(int j = 0; j < panoDiff.rows; j++){
            if(isPixelEmpty(pano0.at<cv::Vec3b>(j, i)) && isPixelEmpty(pano1.at<cv::Vec3b>(j, i)))
                continue;
            if(isPixelEmpty(pano0.at<cv::Vec3b>(j, i)) || isPixelEmpty(pano1.at<cv::Vec3b>(j, i))){
                panoDiff.at<cv::Vec3b>(j, i) = cv::Vec3b(255, 255, 255);
                continue;
            }
            cv::Vec3b panoCol0 = pano0.at<cv::Vec3b>(j, i);
            cv::Vec3b panoCol1 = pano1.at<cv::Vec3b>(j, i);
            float grayIntensity0 = 0.2126 * panoCol0[0] + 0.7152 * panoCol0[1] + 0.0722 * panoCol0[2];
            float grayIntensity1 = 0.2126 * panoCol1[0] + 0.7152 * panoCol1[1] + 0.0722 * panoCol1[2];
            panoDiff.at<cv::Vec3b>(j, i) = cv::Vec3b(std::abs(panoCol0[0] - panoCol1[0]), std::abs(panoCol0[1] - panoCol1[1]), std::abs(panoCol0[2] - panoCol1[2]));
        }
    }

//    float maxblue = 0;
//    float maxred = 0;
//    float maxgreen = 0;
//    for(int i = 0; i < panoDiff.cols; i++){
//        for(int j = 0; j < panoDiff.rows; j++){
//            cv::Vec3b panoCol = panoDiff.at<cv::Vec3b>(j, i);
//            if(isPixelEmpty(panoCol))
//                continue;
//            if(panoCol[0] == 225 && panoCol[1] == 225 && panoCol[2] == 225)
//                continue;
//            maxblue = std::max((float)panoCol[0], maxblue);
//            maxred = std::max((float)panoCol[1], maxred);
//            maxgreen = std::max((float)panoCol[2], maxgreen);
//        }
//    }
//
//    float bluediff = 255 - maxblue;
//    float reddiff = 255 - maxred;
//    float greendiff = 255 - maxgreen;
//
//
//    for(int i = 0; i < panoDiff.cols; i++){
//        for(int j = 0; j < panoDiff.rows; j++){
//            cv::Vec3b panoCol = panoDiff.at<cv::Vec3b>(j, i);
//            if(isPixelEmpty(panoCol))
//                continue;
//            if(panoCol[0] == 225 && panoCol[1] == 225 && panoCol[2] == 225)
//                continue;
//            panoDiff.at<cv::Vec3b>(j, i) = cv::Vec3b(panoCol[0] + bluediff, panoCol[1] + reddiff, panoCol[2] + greendiff);
//        }
//    }

    cv::imwrite(resultsDir + "5panoDiff.jpg", panoDiff);
}


int main() {
    try {
        run("1_hanging"); // TODO 3 проанализируйте результаты по фотографиям с дрона - где различие сильное, где малое? почему так?
        run("2_hiking"); // TODO 4 проанализируйте результаты по фотографиям с дрона - где различие сильное, где малое? почему так?
        run("3_aero"); // TODO 5 проанализируйте результаты по фотографиям с дрона - где различие сильное, где малое? почему так?
        run("4_your_data"); // TODO 6 сфотографируйте что-нибудь сами при этом на второй картинке что-то изменив, проведите анализ
        // TODO 7 проведите анализ результатов на базе Вопросов-Упражнений предложенных в последней статье "Урок 19: панорама и визуализация качества склейки"

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}
