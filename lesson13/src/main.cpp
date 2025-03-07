#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <iostream>
#include <filesystem>

#include <libutils/rasserts.h>


std::vector<cv::Point2f> filterPoints(std::vector<cv::Point2f> points, std::vector<unsigned char> matchIsGood) {
    rassert(points.size() == matchIsGood.size(), 234827348927016);

    std::vector<cv::Point2f> goodPoints;
    for (int i = 0; i < matchIsGood.size(); ++i) {
        if (matchIsGood[i]) {
            goodPoints.push_back(points[i]);
        }
    }
    return goodPoints;
}


void test1() {
    std::string caseName = "1_box2";

    std::string path = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\data\\" + caseName + "\\";
    std::string results = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\resultsData\\" + caseName + "\\";

    if (!std::filesystem::exists(results)) { // если папка для результатов еще не создана
        std::filesystem::create_directory(results); // то создаем ее
    }

    cv::Mat img0 = cv::imread(path + "box0.png");
    cv::Mat img1 = cv::imread(path + "box1.png");
    cv::Mat img1_alternative = cv::imread(path + "box1_nesquik.png");
    rassert(!img0.empty() && !img1.empty() && !img1_alternative.empty(), 2389827851080019);
    rassert(img0.type() == CV_8UC3, 2389827851080020);
    rassert(img1.type() == CV_8UC3, 2389827851080021);
    rassert(img1_alternative.type() == CV_8UC3, 2389827851080023);

    // Этот объект - алгоритм SIFT (детектирования и описания ключевых точек)
    cv::Ptr<cv::FeatureDetector> detector = cv::SIFT::create();

    // Детектируем и описываем ключевые точки
    std::vector<cv::KeyPoint> keypoints0, keypoints1; // здесь будет храниться список ключевых точек
    cv::Mat descriptors0, descriptors1; // здесь будут зраниться дескрипторы этих ключевых точек
    std::cout << "Detecting SIFT keypoints and describing them (computing their descriptors)..." << std::endl;
    detector->detectAndCompute(img0, cv::noArray(), keypoints0, descriptors0);
    detector->detectAndCompute(img1, cv::noArray(), keypoints1, descriptors1);
    std::cout << "SIFT keypoints detected and described: " << keypoints0.size() << " and " << keypoints1.size() << std::endl;

    {
        // Давайте нарисуем на картинке где эти точки были обнаружены для визуализации
        cv::Mat img0withKeypoints, img1withKeypoints;
        cv::drawKeypoints(img0, keypoints0, img0withKeypoints);
        cv::drawKeypoints(img1, keypoints1, img1withKeypoints);
        cv::imwrite(results + "01keypoints0.jpg", img0withKeypoints);
        cv::imwrite(results + "01keypoints1.jpg", img1withKeypoints);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 0 и картинкой 1:
    // найдя для каждой точки из первой картинки - ДВЕ самые похожие точки из второй картинки
    std::vector<std::vector<cv::DMatch>> matches01;
    std::cout << "Matching " << keypoints0.size() << " points with " << keypoints1.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors0, descriptors1, matches01, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints0.size() == matches01.size(), 234728972980049);
    for (int i = 0; i < matches01.size(); ++i) {
        rassert(matches01[i].size() == 2, 3427890347902051);
        rassert(matches01[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches01[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches01[i][0].trainIdx < keypoints1.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches01[i][1].trainIdx < keypoints1.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches01[i][0].distance <= matches01[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }

    {
        std::vector<double> distances;
        for (int i = 0; i < matches01.size(); ++i) {
            distances.push_back(matches01[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches01 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
    }

    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches01.size(); ++i) {
            matchesK.push_back(matches01[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img0, keypoints0, img1, keypoints1, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches01_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 1 и картинкой 0 (т.е. в обратную сторону):
    std::vector<std::vector<cv::DMatch>> matches10;
    std::cout << "Matching " << keypoints1.size() << " points with " << keypoints0.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher1 = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors1, descriptors0, matches10, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints1.size() == matches10.size(), 234728972980049);
    for (int i = 0; i < matches10.size(); ++i) {
        rassert(matches10[i].size() == 2, 3427890347902051);
        rassert(matches10[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches10[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches10[i][0].trainIdx < keypoints0.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches10[i][1].trainIdx < keypoints0.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches10[i][0].distance <= matches10[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }

    double median;
    {
        std::vector<double> distances;
        for (int i = 0; i < matches10.size(); ++i) {
            distances.push_back(matches10[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches10 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
        median = distances[distances.size()/2]*0.8;
    }
    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches10.size(); ++i) {
            matchesK.push_back(matches10[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img1, keypoints1, img0, keypoints0, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches10_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }


    // Теперь давайте попробуем убрать ошибочные сопоставления
    std::cout << "Filtering matches..." << std::endl;
    std::vector<cv::Point2f> points0, points1; // здесь сохраним координаты ключевых точек для удобства позже
    std::vector<unsigned char> matchIsGood; // здесь мы отметим true - хорошие сопоставления, и false - плохие
    int nmatchesGood = 0; // посчитаем сколько сопоставлений посчиталось хорошими
    for (int i = 0; i < keypoints0.size(); ++i) {
        cv::DMatch match = matches01[i][0];
        rassert(match.queryIdx == i, 234782749278097); // и вновь - queryIdx это откуда точки (поэтому всегда == i)
        int j = match.trainIdx; // и trainIdx - это какая точка из второго массива точек оказалась к нам (к queryIdx из первого массива точек) ближайшей
        rassert(j < keypoints1.size(), 38472957238099); // поэтому явно проверяем что индекс не вышел за пределы второго массива точек

        points0.push_back(keypoints0[i].pt);
        points1.push_back(keypoints1[j].pt);

        bool isOk = true;

        if (match.distance > median) {
            isOk = false;
        }

        cv::DMatch match2 = matches01[i][1];
        if (match.distance > 0.7*match2.distance) {
            isOk = false;
        }

//         добавьте left-right check, т.е. проверку правда ли если для точки А самой похожей оказалась точка Б, то вероятно при обратном сопоставлении и у точки Б - ближайшей является точка А
//        cv::DMatch match01 = match;
//        cv::DMatch match10 = matches10[match01.trainIdx][0];
//        if (match10.trainIdx!=i) {
//            isOk = false;
//        }

        // TODO: визуализация в 04goodMatches01.jpg покажет вам какие сопоставления остаются, какой из этих методов фильтрации оказался лучше всего?
        // TODO: попробуйте оставить каждый из них закомменьтировав два других, какой самый крутой?
        // TODO: попробуйте решить какую комбинацию этих методов вам хотелось бы использовать в результате?
        // TODO: !!!ОБЯЗАТЕЛЬНО!!! ЗАПИШИТЕ СЮДА ВВИДЕ КОММЕНТАРИЯ СВОИ ОТВЕТЫ НА ЭТИ ВОПРОСЫ И СВОИ ВЫВОДЫ!!!

        // получилось, что лучше работает комбинация первых двух
        if (isOk) {
            ++nmatchesGood;
            matchIsGood.push_back(true);
        } else {
            matchIsGood.push_back(false);
        }
    }
    rassert(points0.size() == points1.size(), 3497282579850108);
    rassert(points0.size() == matchIsGood.size(), 3497282579850109);
    std::cout << nmatchesGood << "/" << keypoints0.size() << " good matches left" << std::endl;

    {
        std::vector<cv::DMatch> goodMatches;
        for (int i = 0; i < matches01.size(); ++i) {
            cv::DMatch match = matches01[i][0];
            rassert(match.queryIdx == i, 2347982739280182);
            rassert(match.trainIdx < points1.size(), 2347982739280182);
            if (!matchIsGood[i])
                continue;

            goodMatches.push_back(match);
        }
        cv::Mat imgWithMatches;
        // визуализируем хорошие соопставления
        cv::drawMatches(img0, keypoints0, img1, keypoints1, goodMatches, imgWithMatches);
        cv::imwrite(results + "04goodMatches01.jpg", imgWithMatches);
    }

    // Теперь на базе оставшихся хороших сопоставлений - воспользуемся готовым методом RANSAC в OpenCV чтобы найти матрицу преобразования из первой картинки во вторую
    std::cout << "Finding Homography matrix from image0 to image1..." << std::endl;
    const double ransacReprojThreshold = 3.0;
    std::vector<cv::Point2f> pointsGood0 = filterPoints(points0, matchIsGood);
    std::vector<cv::Point2f> pointsGood1 = filterPoints(points1, matchIsGood);
    std::vector<unsigned char> inliersMask; // в этот вектор RANSAC запишет флажки - какие сопоставления он посчитал корректными (inliers)
    // RANSAC:
    cv::Mat H01 = cv::findHomography(pointsGood0, pointsGood1, cv::RANSAC, ransacReprojThreshold, inliersMask);
    // H01 - матрица 3х3 описывающая преобразование плоскости первой картинки в плоскость второй картинки
    std::cout << "homography matrix found:" << std::endl;
    std::cout << H01 << std::endl;
    {
        std::vector<cv::DMatch> inliersMatches;
        std::vector<cv::KeyPoint> inlierKeypoints0, inlierKeypoints1;

        int ninliers = 0;
        for (int i = 0; i < inliersMask.size(); ++i) {
            if (inliersMask[i]) {
                ++ninliers;
                cv::DMatch match;
                match.queryIdx = inlierKeypoints0.size();
                match.trainIdx = inlierKeypoints1.size();
                inliersMatches.push_back(match);
                inlierKeypoints0.push_back(cv::KeyPoint(pointsGood0[i], 3.0));
                inlierKeypoints1.push_back(cv::KeyPoint(pointsGood1[i], 3.0));
            }
        }

        // визуализируем inliersMask - т.е. какие сопоставления в конечном счете RANSAC посчитал корректными (т.е. не выбросами, не outliers)
        std::cout << "(with " << ninliers << "/" << nmatchesGood << " inliers matches)" << std::endl;
        cv::Mat imgWithInliersMatches;
        cv::drawMatches(img0, inlierKeypoints0, img1, inlierKeypoints1, inliersMatches, imgWithInliersMatches);
        cv::imwrite(results + "05inliersMatches01.jpg", imgWithInliersMatches);
    }

    cv::imwrite(results + "06img1.jpg", img1); // сохраняем вторую картинку

    cv::Mat img0to1;
    cv::warpPerspective(img0, img0to1, H01, img1.size()); // преобразуем первую картинку соответственно матрице преобразования
    cv::imwrite(results + "07img0to1.jpg", img0to1);



    cv::imwrite(results + "08img0.jpg", img0); // сохраним первую картинку

    cv::Mat H10 = H01.inv(); // у матрицы есть обратная матрица - находим ее, какое преобразование она делает?
    cv::Mat img1to0;
    cv::warpPerspective(img1, img1to0, H10, img0.size());
    cv::imwrite(results + "09img1to0.jpg", img1to0);

    img1to0 = img0.clone(); // давайте теперь вторую картинку нарисуем не просто в пространстве первой картинки - но поверх нее!
    cv::warpPerspective(img1, img1to0, H10, img1to0.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    cv::imwrite(results + "10img0with1to0.jpg", img1to0);

    img1to0 = img0.clone();
    cv::warpPerspective(img1_alternative, img1to0, H10, img1to0.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT); // сделайте то же самое что и в предыдущей визуализации но вместо второй картинки - наложите картинку с несквиком
    cv::imwrite(results + "11img0withNesquik.jpg", img1to0);
}

void test2() {
    std::string caseName = "2_hiking2";

    std::string path = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\data\\" + caseName + "\\";
    std::string results = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\resultsData\\" + caseName + "\\";

    if (!std::filesystem::exists(results)) { // если папка для результатов еще не создана
        std::filesystem::create_directory(results); // то создаем ее
    }

    cv::Mat img0 = cv::imread(path + "hiking0.png");
    cv::Mat img1 = cv::imread(path + "hiking1.png");
    rassert(!img0.empty() && !img1.empty(), 2389827851080019);
    rassert(img0.type() == CV_8UC3, 2389827851080020);
    rassert(img1.type() == CV_8UC3, 2389827851080021);

    // Этот объект - алгоритм SIFT (детектирования и описания ключевых точек)
    cv::Ptr<cv::FeatureDetector> detector = cv::SIFT::create();

    // Детектируем и описываем ключевые точки
    std::vector<cv::KeyPoint> keypoints0, keypoints1; // здесь будет храниться список ключевых точек
    cv::Mat descriptors0, descriptors1; // здесь будут зраниться дескрипторы этих ключевых точек
    std::cout << "Detecting SIFT keypoints and describing them (computing their descriptors)..." << std::endl;
    detector->detectAndCompute(img0, cv::noArray(), keypoints0, descriptors0);
    detector->detectAndCompute(img1, cv::noArray(), keypoints1, descriptors1);
    std::cout << "SIFT keypoints detected and described: " << keypoints0.size() << " and " << keypoints1.size() << std::endl;

    {
        // Давайте нарисуем на картинке где эти точки были обнаружены для визуализации
        cv::Mat img0withKeypoints, img1withKeypoints;
        cv::drawKeypoints(img0, keypoints0, img0withKeypoints);
        cv::drawKeypoints(img1, keypoints1, img1withKeypoints);
        cv::imwrite(results + "01keypoints0.jpg", img0withKeypoints);
        cv::imwrite(results + "01keypoints1.jpg", img1withKeypoints);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 0 и картинкой 1:
    // найдя для каждой точки из первой картинки - ДВЕ самые похожие точки из второй картинки
    std::vector<std::vector<cv::DMatch>> matches01;
    std::cout << "Matching " << keypoints0.size() << " points with " << keypoints1.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors0, descriptors1, matches01, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints0.size() == matches01.size(), 234728972980049);
    for (int i = 0; i < matches01.size(); ++i) {
        rassert(matches01[i].size() == 2, 3427890347902051);
        rassert(matches01[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches01[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches01[i][0].trainIdx < keypoints1.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches01[i][1].trainIdx < keypoints1.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches01[i][0].distance <= matches01[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }


    {
        std::vector<double> distances;
        for (int i = 0; i < matches01.size(); ++i) {
            distances.push_back(matches01[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches01 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
    }
    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches01.size(); ++i) {
            matchesK.push_back(matches01[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img0, keypoints0, img1, keypoints1, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches01_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 1 и картинкой 0 (т.е. в обратную сторону):
    std::vector<std::vector<cv::DMatch>> matches10;
    std::cout << "Matching " << keypoints1.size() << " points with " << keypoints0.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher1 = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors1, descriptors0, matches10, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints1.size() == matches10.size(), 234728972980049);
    for (int i = 0; i < matches10.size(); ++i) {
        rassert(matches10[i].size() == 2, 3427890347902051);
        rassert(matches10[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches10[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches10[i][0].trainIdx < keypoints0.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches10[i][1].trainIdx < keypoints0.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches10[i][0].distance <= matches10[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }

    double med;
    {
        std::vector<double> distances;
        for (int i = 0; i < matches10.size(); ++i) {
            distances.push_back(matches10[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches10 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
        med = distances[distances.size()/2]*0.8;
    }
    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches10.size(); ++i) {
            matchesK.push_back(matches10[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img1, keypoints1, img0, keypoints0, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches10_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }

    // Теперь давайте попробуем убрать ошибочные сопоставления
    std::cout << "Filtering matches..." << std::endl;
    std::vector<cv::Point2f> points0, points1; // здесь сохраним координаты ключевых точек для удобства позже
    std::vector<unsigned char> matchIsGood; // здесь мы отметим true - хорошие сопоставления, и false - плохие
    int nmatchesGood = 0; // посчитаем сколько сопоставлений посчиталось хорошими
    for (int i = 0; i < keypoints0.size(); ++i) {
        cv::DMatch match = matches01[i][0];
        rassert(match.queryIdx == i, 234782749278097); // и вновь - queryIdx это откуда точки (поэтому всегда == i)
        int j = match.trainIdx; // и trainIdx - это какая точка из второго массива точек оказалась к нам (к queryIdx из первого массива точек) ближайшей
        rassert(j < keypoints1.size(), 38472957238099); // поэтому явно проверяем что индекс не вышел за пределы второго массива точек

        points0.push_back(keypoints0[i].pt);
        points1.push_back(keypoints1[j].pt);

        bool isOk = true;

        if (match.distance > med) {
            isOk = false;
        }

        cv::DMatch match2 = matches01[i][1];
        if (match.distance > 0.7*match2.distance) {
            isOk = false;
        }

//        cv::DMatch match01 = match;
//        cv::DMatch match10 = matches10[match01.trainIdx][0];
//        if (match10.trainIdx!=i) {
//            isOk = false;
//        }


        // TODO: V ОТВЕТЫ НА ВОПРОСЫ V
        // лучше всего k-ratio
        // попробовав все комюинации, содержащие k-ratio, получилось что лучше работает комбинация первых 2
        // по хорошему, надо бы перебрать гиперпараметры (treshold, K),
        // взять взвешенную сумму критериев, и выкидывать картинки с суммой>1 и перебрать веса для каждого из 3 критериев
        // а потом вывести на экран получившиеся, допустим, 30 картинок с разными параметрами, и выбрать лучшую
        // но лень, поэтому пока оставлю так
        if (isOk) {
            ++nmatchesGood;
            matchIsGood.push_back(true);
        } else {
            matchIsGood.push_back(false);
        }
    }
    rassert(points0.size() == points1.size(), 3497282579850108);
    rassert(points0.size() == matchIsGood.size(), 3497282579850109);
    std::cout << nmatchesGood << "/" << keypoints0.size() << " good matches left" << std::endl;

    {
        std::vector<cv::DMatch> goodMatches;
        for (int i = 0; i < matches01.size(); ++i) {
            cv::DMatch match = matches01[i][0];
            rassert(match.queryIdx == i, 2347982739280182);
            rassert(match.trainIdx < points1.size(), 2347982739280182);
            if (!matchIsGood[i])
                continue;

            goodMatches.push_back(match);
        }
        cv::Mat imgWithMatches;
        // визуализируем хорошие соопставления
        cv::drawMatches(img0, keypoints0, img1, keypoints1, goodMatches, imgWithMatches);
        cv::imwrite(results + "04goodMatches01.jpg", imgWithMatches);
    }

    // Теперь на базе оставшихся хороших сопоставлений - воспользуемся готовым методом RANSAC в OpenCV чтобы найти матрицу преобразования из первой картинки во вторую
    std::cout << "Finding Homography matrix from image0 to image1..." << std::endl;
    const double ransacReprojThreshold = 3.0;
    std::vector<cv::Point2f> pointsGood0 = filterPoints(points0, matchIsGood);
    std::vector<cv::Point2f> pointsGood1 = filterPoints(points1, matchIsGood);
    std::vector<unsigned char> inliersMask; // в этот вектор RANSAC запишет флажки - какие сопоставления он посчитал корректными (inliers)
    // RANSAC:
    cv::Mat H01 = cv::findHomography(pointsGood0, pointsGood1, cv::RANSAC, ransacReprojThreshold, inliersMask);
    // H01 - матрица 3х3 описывающая преобразование плоскости первой картинки в плоскость второй картинки
    std::cout << "homography matrix found:" << std::endl;
    std::cout << H01 << std::endl;
    {
        std::vector<cv::DMatch> inliersMatches;
        std::vector<cv::KeyPoint> inlierKeypoints0, inlierKeypoints1;

        int ninliers = 0;
        for (int i = 0; i < inliersMask.size(); ++i) {
            if (inliersMask[i]) {
                ++ninliers;
                cv::DMatch match;
                match.queryIdx = inlierKeypoints0.size();
                match.trainIdx = inlierKeypoints1.size();
                inliersMatches.push_back(match);
                inlierKeypoints0.push_back(cv::KeyPoint(pointsGood0[i], 3.0));
                inlierKeypoints1.push_back(cv::KeyPoint(pointsGood1[i], 3.0));
            }
        }

        // визуализируем inliersMask - т.е. какие сопоставления в конечном счете RANSAC посчитал корректными (т.е. не выбросами, не outliers)
        std::cout << "(with " << ninliers << "/" << nmatchesGood << " inliers matches)" << std::endl;
        cv::Mat imgWithInliersMatches;
        cv::drawMatches(img0, inlierKeypoints0, img1, inlierKeypoints1, inliersMatches, imgWithInliersMatches);
        cv::imwrite(results + "05inliersMatches01.jpg", imgWithInliersMatches);
    }

    cv::imwrite(results + "06img1.jpg", img1); // сохраняем вторую картинку

    cv::Mat img0to1;
    cv::warpPerspective(img0, img0to1, H01, img1.size()); // преобразуем первую картинку соответственно матрице преобразования
    cv::imwrite(results + "07img0to1.jpg", img0to1);

    double diff = 0;
    for(int i = 0; i<img1.rows; i++){
        for(int j = 0; j<img1.cols; j++){
            cv::Vec3b col1, col2;
            col1 = img1.at<cv::Vec3b>(i, j);
            col2 = img0to1.at<cv::Vec3b>(i, j);
            diff+=abs(col1[0]-col2[0])+abs(col1[1]-col2[1])+abs(col1[2]-col2[2]);
        }
    }
    rassert(diff<img1.rows*img1.cols*400, "big difference");
    cv::imwrite(results + "08img0.jpg", img0); // сохраним первую картинку

    cv::Mat H10 = H01.inv(); // у матрицы есть обратная матрица - находим ее, какое преобразование она делает?
    cv::Mat img1to0;
    cv::warpPerspective(img1, img1to0, H10, img0.size());
    cv::imwrite(results + "09img1to0.jpg", img1to0);

    //вычисляем новые размеры изображения, преобразовывая углы 2 изображения
    std::vector<cv::Point2f> inputCorners(4);
    inputCorners[0]=cv::Point2f(0, 0);
    inputCorners[1]=cv::Point2f(img1.cols, 0);
    inputCorners[2]=cv::Point2f(0, img1.rows);
    inputCorners[3]=cv::Point2f(img1.cols, img1.rows);

    std::vector<cv::Point2f> outputCorners(4);
    perspectiveTransform(inputCorners, outputCorners, H10);

    outputCorners.push_back(cv::Point2f(0, 0));
    outputCorners.push_back(cv::Point2f(img0.cols, 0));
    outputCorners.push_back(cv::Point2f(0, img0.rows));
    outputCorners.push_back(cv::Point2f(img0.cols, img0.rows));

    cv::Rect br= boundingRect(outputCorners);

    cv::Mat img0_resized(br.size(), CV_8UC3);
    img1to0 = img0.clone(); // давайте теперь вторую картинку нарисуем не просто в пространстве первой картинки - но поверх нее!
    img1to0.copyTo(img0_resized(cv::Rect(-br.tl().x, -br.tl().y, img1to0.cols, img1to0.rows)));
    cv::imwrite(results + "10img0resized.jpg", img0_resized);
    cv::warpPerspective(img1, img0_resized, H10, img0_resized.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    cv::imwrite(results + "11img0with1to0.jpg", img0_resized);
}

void test3() {
    std::string caseName = "3_hanging2";

    std::string path = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\data\\" + caseName + "\\";
    std::string results = "C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson13\\resultsData\\" + caseName + "\\";

    if (!std::filesystem::exists(results)) { // если папка для результатов еще не создана
        std::filesystem::create_directory(results); // то создаем ее
    }

    cv::Mat img0 = cv::imread(path + "hanging0.png");
    cv::Mat img1 = cv::imread(path + "hanging1.png");
    rassert(!img0.empty(), 2389827851080019);
    rassert(!img1.empty(), 2389827851080019);
    rassert(img0.type() == CV_8UC3, 2389827851080020);
    rassert(img1.type() == CV_8UC3, 2389827851080021);

    // Этот объект - алгоритм SIFT (детектирования и описания ключевых точек)
    cv::Ptr<cv::FeatureDetector> detector = cv::SIFT::create();

    // Детектируем и описываем ключевые точки
    std::vector<cv::KeyPoint> keypoints0, keypoints1; // здесь будет храниться список ключевых точек
    cv::Mat descriptors0, descriptors1; // здесь будут зраниться дескрипторы этих ключевых точек
    std::cout << "Detecting SIFT keypoints and describing them (computing their descriptors)..." << std::endl;
    detector->detectAndCompute(img0, cv::noArray(), keypoints0, descriptors0);
    detector->detectAndCompute(img1, cv::noArray(), keypoints1, descriptors1);
    std::cout << "SIFT keypoints detected and described: " << keypoints0.size() << " and " << keypoints1.size() << std::endl;

    {
        // Давайте нарисуем на картинке где эти точки были обнаружены для визуализации
        cv::Mat img0withKeypoints, img1withKeypoints;
        cv::drawKeypoints(img0, keypoints0, img0withKeypoints);
        cv::drawKeypoints(img1, keypoints1, img1withKeypoints);
        cv::imwrite(results + "01keypoints0.jpg", img0withKeypoints);
        cv::imwrite(results + "01keypoints1.jpg", img1withKeypoints);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 0 и картинкой 1:
    // найдя для каждой точки из первой картинки - ДВЕ самые похожие точки из второй картинки
    std::vector<std::vector<cv::DMatch>> matches01;
    std::cout << "Matching " << keypoints0.size() << " points with " << keypoints1.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors0, descriptors1, matches01, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints0.size() == matches01.size(), 234728972980049);
    for (int i = 0; i < matches01.size(); ++i) {
        rassert(matches01[i].size() == 2, 3427890347902051);
        rassert(matches01[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches01[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches01[i][0].trainIdx < keypoints1.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches01[i][1].trainIdx < keypoints1.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches01[i][0].distance <= matches01[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }


    {
        std::vector<double> distances;
        for (int i = 0; i < matches01.size(); ++i) {
            distances.push_back(matches01[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches01 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
    }
    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches01.size(); ++i) {
            matchesK.push_back(matches01[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img0, keypoints0, img1, keypoints1, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches01_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }

    // Теперь давайте сопоставим ключевые точки между картинкой 1 и картинкой 0 (т.е. в обратную сторону):
    std::vector<std::vector<cv::DMatch>> matches10;
    std::cout << "Matching " << keypoints1.size() << " points with " << keypoints0.size() << "..." << std::endl;
    cv::Ptr<cv::DescriptorMatcher> matcher1 = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    matcher->knnMatch(descriptors1, descriptors0, matches10, 2); // k: 2 - указывает что мы ищем ДВЕ ближайшие точки, а не ОДНУ САМУЮ БЛИЖАЙШУЮ
    std::cout << "matching done" << std::endl;
    // т.к. мы для каждой точки keypoints0 ищем ближайшую из keypoints1, то сопоставлений найдено столько же сколько точек в keypoints0:
    rassert(keypoints1.size() == matches10.size(), 234728972980049);
    for (int i = 0; i < matches10.size(); ++i) {
        rassert(matches10[i].size() == 2, 3427890347902051);
        rassert(matches10[i][0].queryIdx == i, 237812974128941); // queryIdx - это индекс ключевой точки в первом векторе точек, т.к. мы для всех точек keypoints0
        rassert(matches10[i][1].queryIdx == i, 237812974128942); // ищем ближайшую в keypoints1, queryIdx == i, т.е. равен индексу очередной точки keypoints0

        rassert(matches10[i][0].trainIdx < keypoints0.size(), 237812974128943); // trainIdx - это индекс точки в keypoints1 самой похожей на keypoints0[i]
        rassert(matches10[i][1].trainIdx < keypoints0.size(), 237812974128943); // а этот trainIdx - это индекс точки в keypoints1 ВТОРОЙ по похожести на keypoints0[i]

        rassert(matches10[i][0].distance <= matches10[i][1].distance, 328493778); // давайте явно проверим что расстояние для этой второй точки - не меньше чем для первой точки
    }

    double med;
    {
        std::vector<double> distances;
        for (int i = 0; i < matches10.size(); ++i) {
            distances.push_back(matches10[i][0].distance);
        }
        std::sort(distances.begin(), distances.end()); // GOOGLE: "cpp how to sort vector"
        std::cout << "matches10 distances min/median/max: " << distances[0] << "/" << distances[distances.size()/2] << "/" << distances[distances.size()-1] << std::endl;
        med = distances[distances.size()/2]*0.8;
    }
    for (int k = 0; k < 2; ++k) {
        std::vector<cv::DMatch> matchesK;
        for (int i = 0; i < matches10.size(); ++i) {
            matchesK.push_back(matches10[i][k]);
        }
        // давайте взглянем как выглядят сопоставления между точками (k - указывает на какие сопоставления мы сейчас смотрим, на ближайшие, или на вторые по близости)
        cv::Mat imgWithMatches;
        cv::drawMatches(img1, keypoints1, img0, keypoints0, matchesK, imgWithMatches);
        cv::imwrite(results + "02matches10_k" + std::to_string(k) + ".jpg", imgWithMatches);
    }

    // Теперь давайте попробуем убрать ошибочные сопоставления
    std::cout << "Filtering matches..." << std::endl;
    std::vector<cv::Point2f> points0, points1; // здесь сохраним координаты ключевых точек для удобства позже
    std::vector<unsigned char> matchIsGood; // здесь мы отметим true - хорошие сопоставления, и false - плохие
    int nmatchesGood = 0; // посчитаем сколько сопоставлений посчиталось хорошими
    for (int i = 0; i < keypoints0.size(); ++i) {
        cv::DMatch match = matches01[i][0];
        rassert(match.queryIdx == i, 234782749278097); // и вновь - queryIdx это откуда точки (поэтому всегда == i)
        int j = match.trainIdx; // и trainIdx - это какая точка из второго массива точек оказалась к нам (к queryIdx из первого массива точек) ближайшей
        rassert(j < keypoints1.size(), 38472957238099); // поэтому явно проверяем что индекс не вышел за пределы второго массива точек

        points0.push_back(keypoints0[i].pt);
        points1.push_back(keypoints1[j].pt);

        bool isOk = true;

        if (match.distance > med) {
            isOk = false;
        }

        cv::DMatch match2 = matches01[i][1];
        if (match.distance > 0.7*match2.distance) {
            isOk = false;
        }

//        cv::DMatch match01 = match;
//        cv::DMatch match10 = matches10[match01.trainIdx][0];
//        if (match10.trainIdx!=i) {
//            isOk = false;
//        }


        // TODO: V ОТВЕТЫ НА ВОПРОСЫ V
        // лучше всего k-ratio
        // попробовав все комюинации, содержащие k-ratio, получилось что лучше работает комбинация первых 2
        // по хорошему, надо бы перебрать гиперпараметры (treshold, K),
        // взять взвешенную сумму критериев, и выкидывать картинки с суммой>1 и перебрать веса для каждого из 3 критериев
        // а потом вывести на экран получившиеся, допустим, 30 картинок с разными параметрами, и выбрать лучшую
        // но лень, поэтому пока оставлю так
        if (isOk) {
            ++nmatchesGood;
            matchIsGood.push_back(true);
        } else {
            matchIsGood.push_back(false);
        }
    }
    rassert(points0.size() == points1.size(), 3497282579850108);
    rassert(points0.size() == matchIsGood.size(), 3497282579850109);
    std::cout << nmatchesGood << "/" << keypoints0.size() << " good matches left" << std::endl;

    {
        std::vector<cv::DMatch> goodMatches;
        for (int i = 0; i < matches01.size(); ++i) {
            cv::DMatch match = matches01[i][0];
            rassert(match.queryIdx == i, 2347982739280182);
            if(match.trainIdx >= points1.size())
                continue;
            if (!matchIsGood[i])
                continue;

            goodMatches.push_back(match);
        }
        cv::Mat imgWithMatches;
        // визуализируем хорошие соопставления
        cv::drawMatches(img0, keypoints0, img1, keypoints1, goodMatches, imgWithMatches);
        cv::imwrite(results + "04goodMatches01.jpg", imgWithMatches);
    }

    // Теперь на базе оставшихся хороших сопоставлений - воспользуемся готовым методом RANSAC в OpenCV чтобы найти матрицу преобразования из первой картинки во вторую
    std::cout << "Finding Homography matrix from image0 to image1..." << std::endl;
    const double ransacReprojThreshold = 3.0;
    std::vector<cv::Point2f> pointsGood0 = filterPoints(points0, matchIsGood);
    std::vector<cv::Point2f> pointsGood1 = filterPoints(points1, matchIsGood);
    std::vector<unsigned char> inliersMask; // в этот вектор RANSAC запишет флажки - какие сопоставления он посчитал корректными (inliers)
    // RANSAC:
    cv::Mat H01 = cv::findHomography(pointsGood0, pointsGood1, cv::RANSAC, ransacReprojThreshold, inliersMask);
    // H01 - матрица 3х3 описывающая преобразование плоскости первой картинки в плоскость второй картинки
    std::cout << "homography matrix found:" << std::endl;
    std::cout << H01 << std::endl;
    {
        std::vector<cv::DMatch> inliersMatches;
        std::vector<cv::KeyPoint> inlierKeypoints0, inlierKeypoints1;

        int ninliers = 0;
        for (int i = 0; i < inliersMask.size(); ++i) {
            if (inliersMask[i]) {
                ++ninliers;
                cv::DMatch match;
                match.queryIdx = inlierKeypoints0.size();
                match.trainIdx = inlierKeypoints1.size();
                inliersMatches.push_back(match);
                inlierKeypoints0.push_back(cv::KeyPoint(pointsGood0[i], 3.0));
                inlierKeypoints1.push_back(cv::KeyPoint(pointsGood1[i], 3.0));
            }
        }

        // визуализируем inliersMask - т.е. какие сопоставления в конечном счете RANSAC посчитал корректными (т.е. не выбросами, не outliers)
        std::cout << "(with " << ninliers << "/" << nmatchesGood << " inliers matches)" << std::endl;
        cv::Mat imgWithInliersMatches;
        cv::drawMatches(img0, inlierKeypoints0, img1, inlierKeypoints1, inliersMatches, imgWithInliersMatches);
        cv::imwrite(results + "05inliersMatches01.jpg", imgWithInliersMatches);
    }

    cv::imwrite(results + "06img1.jpg", img1); // сохраняем вторую картинку

    cv::Mat img0to1;
    cv::warpPerspective(img0, img0to1, H01, img1.size()); // преобразуем первую картинку соответственно матрице преобразования
    cv::imwrite(results + "07img0to1.jpg", img0to1);

    double diff = 0;
    for(int i = 0; i<img1.rows; i++){
        for(int j = 0; j<img1.cols; j++){
            cv::Vec3b col1, col2;
            col1 = img1.at<cv::Vec3b>(i, j);
            col2 = img0to1.at<cv::Vec3b>(i, j);
            diff+=abs(col1[0]-col2[0])+abs(col1[1]-col2[1])+abs(col1[2]-col2[2]);
        }
    }
    rassert(diff<img1.rows*img1.cols*400, "big difference");
    cv::imwrite(results + "08img0.jpg", img0); // сохраним первую картинку

    cv::Mat H10 = H01.inv(); // у матрицы есть обратная матрица - находим ее, какое преобразование она делает?
    cv::Mat img1to0;
    cv::warpPerspective(img1, img1to0, H10, img0.size());
    cv::imwrite(results + "09img1to0.jpg", img1to0);

    //вычисляем новые размеры изображения, преобразовывая углы 2 изображения
    std::vector<cv::Point2f> inputCorners(4);
    inputCorners[0]=cv::Point2f(0, 0);
    inputCorners[1]=cv::Point2f(img1.cols, 0);
    inputCorners[2]=cv::Point2f(0, img1.rows);
    inputCorners[3]=cv::Point2f(img1.cols, img1.rows);

    std::vector<cv::Point2f> outputCorners(4);
    perspectiveTransform(inputCorners, outputCorners, H10);

    outputCorners.push_back(cv::Point2f(0, 0));
    outputCorners.push_back(cv::Point2f(img0.cols, 0));
    outputCorners.push_back(cv::Point2f(0, img0.rows));
    outputCorners.push_back(cv::Point2f(img0.cols, img0.rows));

    cv::Rect br= boundingRect(outputCorners);

    cv::Mat img0_resized(br.size(), CV_8UC3);
    img1to0 = img0.clone(); // давайте теперь вторую картинку нарисуем не просто в пространстве первой картинки - но поверх нее!
    img1to0.copyTo(img0_resized(cv::Rect(-br.tl().x, -br.tl().y, img1to0.cols, img1to0.rows)));
    cv::imwrite(results + "10img0resized.jpg", img0_resized);
    cv::warpPerspective(img1, img0_resized, H10, img0_resized.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    cv::imwrite(results + "11img0with1to0.jpg", img0_resized);
}


void test4() {
    std::string caseName = "4_aero9";

    // TODO добровольное, подсказка, если матрица A01 переводит первую картинку во вторую, матрица A12 - переводит вторую картинку в третью, то:
    // cv::Mat A02 = A12 * A01; // матрица A02 - является их композицией и переводит из первой картинки в третью (по сути совершая внутри себя промежуточные переходы)
}


int main() {
    try {
        test1(); // TODO обязательное
//        test2(); // TODO обязательное
//        test3(); // TODO обязательное

//        test4(); // TODO добровольный бонус

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}