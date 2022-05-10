#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <set>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <memory>
#include <random>

#include <libutils/rasserts.h>
#include <libutils/fast_random.h>

using namespace std;
using namespace cv;

// Эта функция говорит нам правда ли пиксель отмаскирован, т.е. отмечен как "удаленный", т.е. белый
bool isPixelMasked(cv::Mat mask, int i, int j) {
    rassert(i >= 0 && i < mask.rows, 372489347280017);
    rassert(j >= 0 && j < mask.cols, 372489347280018);
    rassert(mask.type() == CV_8UC3, 2348732984792380019);
    Vec3b color = mask.at<Vec3b>(i, j);
    if(color[0]+color[1]+color[2]>100)
        return 1;
    return 0;
}
default_random_engine generator;
normal_distribution<double> rndNorm(5.0,2.0);
int getNorm(int w, int x0){
    double gen = (rndNorm(generator)/10-0.5)*w;
    while(x0+int(gen)<0 || x0+int(gen)>=w){
        gen = (rndNorm(generator)/10-0.5)*w;
    }
    return x0+int(gen);
}
mt19937 rnd(42);
void run(int caseNumber, std::string caseName) {
    int test[20];
    for(int i = 0; i<20; i++)
        test[i] = 0;
    for(int i = 0; i<20000; i++){
        int temp = getNorm(20, 10);
        if(temp>=0 && temp<20)
            test[temp]++;
    }
    cout << "Normal distribution: ";
    for(int i = 0; i<20; i++)
        cout << test[i] << " ";
    cout << "\n";
    std::cout << "_________Case #" << caseNumber << ": " <<  caseName << "_________" << std::endl;

    cv::Mat original = cv::imread("lesson18/data/" + std::to_string(caseNumber) + "_" + caseName + "/" + std::to_string(caseNumber) + "_original.jpg");
    cv::Mat mask = cv::imread("lesson18/data/" + std::to_string(caseNumber) + "_" + caseName + "/" + std::to_string(caseNumber) + "_mask.png");
    rassert(!original.empty(), 324789374290018);
    rassert(!mask.empty(), 378957298420019);

    rassert(mask.cols==original.cols && mask.rows==original.rows, 1283912);
    std::cout << "Image resolution: " << mask.cols << "x" << mask.rows << std::endl;

    // создаем папку в которую будем сохранять результаты - lesson18/resultsData/ИМЯ_НАБОРА/
    std::string resultsDir = "lesson18/resultsData/";
    if (!std::filesystem::exists(resultsDir)) { // если папка еще не создана
        std::filesystem::create_directory(resultsDir); // то создаем ее
    }
    resultsDir += std::to_string(caseNumber) + "_" + caseName + "/";
    if (!std::filesystem::exists(resultsDir)) { // если папка еще не создана
        std::filesystem::create_directory(resultsDir); // то создаем ее
    }
    vector<pair<Mat, Mat>> pyramid;
    Mat img = original.clone();
    const int PYRAMID_MIN_SIZE = 20; // до какой поры уменьшать картинку? давайте уменьшать пока картинка больше 20 пикселей
    while (img.rows > PYRAMID_MIN_SIZE && img.rows > PYRAMID_MIN_SIZE) { // или пока больше (2 * размер окна для оценки качества)
        pyramid.push_back({img.clone(), mask.clone()}); // мы могли бы воспользоваться push_back но мы хотим вставлять картинки в начало вектора
        cv::pyrDown(img, img); // эта функция уменьшает картинку в два раза
        cv::pyrDown(mask, mask); // эта функция уменьшает картинку в два раза
    }
    cv::Mat shiftslast;
    reverse(pyramid.begin(), pyramid.end());
    for(int layer = 0; layer<pyramid.size(); layer++){

        img = pyramid[layer].first;
        mask = pyramid[layer].second;
        string nowdir = resultsDir+"/layer_"+ to_string(layer)+"/";
        if (!std::filesystem::exists(nowdir)) { // если папка еще не создана
            std::filesystem::create_directory(nowdir); // то создаем ее
        }
        // сохраняем в папку с результатами оригинальную картинку и маску
        cv::imwrite(nowdir + "0original.png", img);
        cv::imwrite(nowdir + "1mask.png", mask);
        rassert(mask.cols==img.cols && mask.rows==img.rows, 1283912);
        int cntmasked = 0;
        for(int i = 0; i<mask.rows; i++){
            for(int j = 0; j<mask.cols; j++){
                if(isPixelMasked(mask, i, j)){
                    img.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
                    cntmasked++;
                }
            }
        }
        cout << "Layer " << layer+1 << "/" << pyramid.size() << ": " "Number of masked pixels: " <<  cntmasked << "/" << mask.rows*mask.cols << " = " << cntmasked*100.0/mask.rows/mask.cols << "%\n";

        cv::Mat shifts(img.rows, img.cols, CV_32SC2, cv::Scalar(0, 0)); // матрица хранящая смещения, изначально заполнена парами нулей
        if(layer!=0){
            for(int i = 0; i<mask.rows; i++){
                for(int j = 0; j<mask.cols; j++){
                    shifts.at<Vec2i>(i, j) = Vec2i(shiftslast.at<Vec2i>(i/2, j/2)[0]*2, shiftslast.at<Vec2i>(i/2, j/2)[1]*2);
                }
            }
        }
        const int NofTheories = 60;//кратно 5
        const int iters = 500;////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////params!!!!!!!
        const int kernel = 3;//окно (2*kernel+1 X 2*kernel+1)
        const int emptyloss = 2500;
        const int rectrust = 3;
        for(int iter = 0; iter<iters; iter++) {
            for (int i0 = 0; i0 < mask.rows; i0++) {
                for (int j0 = 0; j0 < mask.cols; j0++) {

                    int i = i0;
                    int j = j0;
                    if (iter % 2){
                        i = mask.rows-1-i;
                        j = mask.cols-1-j;
                    }
                    if(isPixelMasked(mask, i, j)) {
                        vector<pair<int, int>> theories;
                        if (i + 1 < mask.rows && isPixelMasked(mask, i+1, j)) {
                            Vec2i vec = shifts.at<Vec2i>(i + 1, j);
                            theories.push_back({i + 1 + vec[0], j + vec[1]});
                            for (int v = 0; v < NofTheories / 5; v++) {
                                theories.push_back(
                                        {getNorm(mask.rows, i + 1 + vec[0]), getNorm(mask.cols, j + vec[1])});
                            }
                        }
                        if (i - 1 >= 0 && isPixelMasked(mask, i-1, j)) {
                            Vec2i vec = shifts.at<Vec2i>(i - 1, j);
                            theories.push_back({i - 1 + vec[0], j + vec[1]});
                            for (int v = 0; v < NofTheories / 5; v++) {
                                theories.push_back(
                                        {getNorm(mask.rows, i - 1 + vec[0]), getNorm(mask.cols, j + vec[1])});
                            }
                        }
                        if (j - 1 >= 0 && isPixelMasked(mask, i, j-1)) {
                            Vec2i vec = shifts.at<Vec2i>(i, j - 1);
                            theories.push_back({i + vec[0], j - 1 + vec[1]});
                            for (int v = 0; v < NofTheories / 5; v++) {
                                theories.push_back(
                                        {getNorm(mask.rows, i + vec[0]), getNorm(mask.cols, j - 1 + vec[1])});
                            }
                        }
                        if (j + 1 < mask.cols && isPixelMasked(mask, i, j+1)) {
                            Vec2i vec = shifts.at<Vec2i>(i, j + 1);
                            theories.push_back({i + vec[0], j + 1 + vec[1]});
                            for (int v = 0; v < NofTheories / 5; v++) {
                                theories.push_back(
                                        {getNorm(mask.rows, i + vec[0]), getNorm(mask.cols, j + 1 + vec[1])});
                            }
                        }
                        if (true) {
                            Vec2i vec = shifts.at<Vec2i>(i, j);
                            theories.push_back({i + vec[0], j + vec[1]});
                            for (int v = 0; v < NofTheories / 5; v++) {
                                theories.push_back({getNorm(mask.rows, i + vec[0]), getNorm(mask.cols, j + vec[1])});
                            }
                        }

                        int cur = 0;
                        int x = i + shifts.at<Vec2i>(i, j)[0];
                        int y = j + shifts.at<Vec2i>(i, j)[1];
                        for (int di = -kernel; di <= kernel; di++) {
                            for (int dj = -kernel; dj <= kernel; dj++) {
                                if (i + di >= mask.rows || i + di < 0 || j + dj >= mask.cols || j + dj < 0)
                                    continue;
                                else if (x + di >= mask.rows || x + di < 0 || y + dj >= mask.cols || y + dj < 0)
                                    cur += emptyloss;
                                else if (isPixelMasked(mask, x + di, y + dj))
                                    cur = INT_MAX/2;
                                else if (isPixelMasked(mask, i + di, j + dj)) {
                                    Vec2i recpos = shifts.at<Vec2i>(i + di, j + dj);//recursively looking at link
                                    recpos[0] += i + di;
                                    recpos[1] += j + dj;
                                    if (isPixelMasked(mask, recpos[0], recpos[1]))
                                        cur += emptyloss;
                                    else {
                                        Vec3b t1 = img.at<Vec3b>(recpos[0], recpos[1]);
                                        Vec3b t2 = img.at<Vec3b>(x + di, y + dj);
                                        cur += rectrust *
                                               ((t1[0] - t2[0]) * (t1[0] - t2[0]) + (t1[1] - t2[1]) * (t1[1] - t2[1]) +
                                                (t1[2] - t2[2]) * (t1[2] - t2[2]));
                                    }
                                } else {
                                    Vec3b t1 = img.at<Vec3b>(i + di, j + dj);
                                    Vec3b t2 = img.at<Vec3b>(x + di, y + dj);
                                    cur += (t1[0] - t2[0]) * (t1[0] - t2[0]) + (t1[1] - t2[1]) * (t1[1] - t2[1]) +
                                           (t1[2] - t2[2]) * (t1[2] - t2[2]);
                                }
                            }
                        }
                        Mat temp = img.clone();
                        for (pair<int, int> now: theories) {

                            int nw = 0;
                            int x = now.first;
                            int y = now.second;
                            temp.at<Vec3b>(x, y) = Vec3b(0, 255, 0);
                            for (int di = -kernel; di <= kernel; di++) {
                                for (int dj = -kernel; dj <= kernel; dj++) {
                                    if (i + di >= mask.rows || i + di < 0 || j + dj >= mask.cols || j + dj < 0)
                                        continue;
                                    else if (x + di >= mask.rows || x + di < 0 || y + dj >= mask.cols || y + dj < 0)
                                        nw += emptyloss;
                                    else if (isPixelMasked(mask, x + di, y + dj))
                                        nw = INT_MAX/2;
                                    else if (isPixelMasked(mask, i + di, j + dj)) {
                                        Vec2i recpos = shifts.at<Vec2i>(i + di, j + dj);//recursively looking at link
                                        recpos[0] += i + di;
                                        recpos[1] += j + dj;
                                        if (isPixelMasked(mask, recpos[0], recpos[1]))
                                            nw += emptyloss;
                                        else {
                                            Vec3b t1 = img.at<Vec3b>(recpos[0], recpos[1]);
                                            Vec3b t2 = img.at<Vec3b>(x + di, y + dj);
                                            nw += rectrust * ((t1[0] - t2[0]) * (t1[0] - t2[0]) +
                                                              (t1[1] - t2[1]) * (t1[1] - t2[1]) +
                                                              (t1[2] - t2[2]) * (t1[2] - t2[2]));
                                        }
                                    } else {
                                        Vec3b t1 = img.at<Vec3b>(i + di, j + dj);
                                        Vec3b t2 = img.at<Vec3b>(x + di, y + dj);
                                        nw += (t1[0] - t2[0]) * (t1[0] - t2[0]) + (t1[1] - t2[1]) * (t1[1] - t2[1]) +
                                              (t1[2] - t2[2]) * (t1[2] - t2[2]);
                                    }
                                }
                            }
                            if (nw < cur) {
                                shifts.at<Vec2i>(i, j) = Vec2i(x - i, y - j);
                                cur = nw;
                            }
                        }
//                        if(layer==0){
//                            string newdir = nowdir+"/"+ to_string(i)+"_"+to_string(j)+"/";
//                            if (!std::filesystem::exists(newdir)) { // если папка еще не создана
//                                std::filesystem::create_directory(newdir); // то создаем ее
//                            }
//                            cv::imwrite(newdir + "aaa.png", temp);
//                        }
                    }
                }
            }
        }
        shiftslast = shifts.clone();
        Mat res = img.clone();
        for (int i = 0; i < mask.rows; i++) {
            for (int j = 0; j < mask.cols; j++) {
                if(isPixelMasked(mask, i, j)){
                    res.at<Vec3b>(i, j) = img.at<Vec3b>(i+shifts.at<Vec2i>(i, j)[0], j+shifts.at<Vec2i>(i, j)[1]);
                }
            }
        }
        cv::imwrite(nowdir + "2res.png", res);
    }

}


int main() {
    try {
        //run(1, "mic");
        run(2, "flowers");
        //run(3, "baloons");
        //run(4, "brickwall");
        //run(5, "old_photo");
        //run(6, "your_data");

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}