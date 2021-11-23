#include "hough.h"

#include <libutils/rasserts.h>

#include <opencv2/imgproc.hpp>

double toRadians(double degrees)
{
    const double PI = 3.14159265358979323846264338327950288;
    return degrees * PI / 180.0;
}

double det (double a, double b, double c, double d) {
    return a * d - b * c;
}

double estimateR(double x0, double y0, double theta0radians)
{
    double r0 = x0 * cos(theta0radians) + y0 * sin(theta0radians);
    return r0;
}

cv::Mat buildHough(cv::Mat sobel) {// единственный аргумент - это результат свертки Собелем изначальной картинки
    // проверяем что входная картинка - одноканальная и вещественная:
    rassert(sobel.type() == CV_32FC1, 237128273918006);

    int width = sobel.cols;
    int height = sobel.rows;

    // решаем какое максимальное значение у параметра theta в нашем пространстве прямых
    int max_theta = 360;

    // решаем какое максимальное значение у параметра r в нашем пространстве прямых:
    int max_r = std::abs(std::sqrt((double)width*width + (double)height*height)) + 1; // TODO замените это число так как вам кажется правильным (отталкиваясь от разрешения картинки - ее ширины и высоты)

    // создаем картинку-аккумулятор, в которой мы будем накапливать суммарные голоса за прямые
    // так же известна как пространство Хафа
    cv::Mat accumulator(max_r, max_theta, CV_32FC1, 0.0); // TODO подумайте какого разрешения она должна быть и поправьте ее размер

    // TODO не забудьте заполнить эту матрицу-картинку-аккумулятор нулями (очистить)

    // проходим по всем пикселям нашей картинки (уже свернутой оператором Собеля)
    for (int y0 = 0; y0 < height; ++y0) {
        for (int x0 = 0; x0 < width; ++x0) {
            // смотрим на пиксель с координатами (x0, y0)
            float strength = sobel.at<float>(y0, x0);// TODO считайте его "силу градиента" из картинки sobel

            // теперь для текущего пикселя надо найти все возможные прямые которые через него проходят
            // переберем параметр theta по всему возможному диапазону (в градусах):
            for (int theta0 = 0; theta0 < max_theta; ++theta0) {
                // TODO рассчитайте на базе информации о том какие координаты у пикселя - (x0, y0) и какой параметр theta0 мы сейчас рассматриваем
                // TODO обратите внимание что функции sin/cos принимают углы в радианах, поэтому сначала нужно пересчитать theta0 в радианы (воспользуйтесь константой PI)
                const double PI = 3.14159265358979323846264338327950288;
                float r0 = x0 * cos((double)theta0*PI/180)+y0*sin((double)theta0*PI/180);
                if(r0 < 0)
                    continue;
                // TODO теперь рассчитайте координаты пикслея в пространстве Хафа (в картинке-аккумуляторе) соответсвующего параметрам theta0, r0
                int i = round(theta0);
                int j = round(r0);

                // чтобы проверить не вышли ли мы за пределы картинки-аккумулятора - давайте явно это проверим:
                rassert(i >= 0, 237891731289044);
                rassert(i < accumulator.cols, 237891731289045);
                rassert(j >= 0, 237891731289046);
                rassert(j < accumulator.rows, 237891731289047);
                // теперь легко отладить случай выхода за пределы картинки
                // TODO просто поставьте точку остановки внутри rassert:
                // нажмите Ctrl+Shift+N -> rasserts.cpp
                // и поставьте точку остановки на 8 строке: "return line;"

                // TODO и добавьте в картинку-аккумулятор наш голос с весом strength (взятый из картинки свернутой Собелем)
                accumulator.at<float>(j, i) += strength;
            }
        }
    }

    return accumulator;
}

std::vector<PolarLineExtremum> findLocalExtremums(cv::Mat houghSpace)
{
    rassert(houghSpace.type() == CV_32FC1, 234827498237080);

    const int max_theta = 360;
    rassert(houghSpace.cols == max_theta, 233892742893082);
    const int max_r = houghSpace.rows;

    std::vector<PolarLineExtremum> winners;

    float max_accumulated = 0.0f;
    for (int j = 0; j < houghSpace.rows; ++j) {
        for (int i = 0; i < houghSpace.cols; ++i) {
            max_accumulated = std::max(max_accumulated, houghSpace.at<float>(j, i));
        }
    }

    for (int theta = 1; theta < max_theta-1; ++theta) {
        for (int r = 1; r < max_r-1; ++r) {
            bool isExtr = 1;
            for(int di = -1; di <= 1; di++)
            {
                for(int dj = -1; dj <= 1; dj++)
                {
                    if(di == 0 && dj == 0)
                        continue;
                    if(houghSpace.at<float>(r, theta) <= houghSpace.at<float>(r + di, theta + dj))
                        isExtr = 0;
                }
            }
            if (isExtr) {
                PolarLineExtremum line(theta, r, (int)(houghSpace.at<float>(r, theta)*255.0f/max_accumulated));
                winners.push_back(line);
            }
        }
    }

    return winners;
}

std::vector<PolarLineExtremum> filterStrongLines(std::vector<PolarLineExtremum> allLines, double thresholdFromWinner)
{
    std::vector<PolarLineExtremum> strongLines;

    // Эта функция по множеству всех найденных локальных экстремумов (прямых) находит самую популярную прямую
    // и возвращает только вектор из тех прямых, что не сильно ее хуже (набрали хотя бы thresholdFromWinner голосов от победителя, т.е. например половину)
    int mxvotes = 0;

    for(int i = 0; i < allLines.size(); i++) {
        if(allLines[i].votes > mxvotes)
            mxvotes = allLines[i].votes;
    }

    for(int i = 0; i < allLines.size(); i++)
        if(allLines[i].votes >= (int)(thresholdFromWinner * mxvotes))
            strongLines.push_back(allLines[i]);

    return strongLines;
}

cv::Mat drawCirclesOnExtremumsInHoughSpace(cv::Mat houghSpace, std::vector<PolarLineExtremum> lines, int radius)
{
    // TODO Доделайте эту функцию - пусть она скопирует картинку с пространством Хафа и отметит на ней красным кружком указанного радиуса (radius) места где были обнаружены экстремумы (на базе списка прямых)

    // делаем копию картинки с пространством Хафа (чтобы не портить само пространство Хафа)
    cv::Mat houghSpaceWithCrosses = houghSpace.clone();

    // проверяем что пространство состоит из 32-битных вещественных чисел (т.е. картинка одноканальная)
    rassert(houghSpaceWithCrosses.type() == CV_32FC1, 347823472890137);

    // но мы хотим рисовать КРАСНЫЙ кружочек вокруг найденных экстремумов, а значит нам не подходит черно-белая картинка
    // поэтому ее надо преобразовать в обычную цветную BGR картинку
    cv::cvtColor(houghSpaceWithCrosses, houghSpaceWithCrosses, cv::COLOR_GRAY2BGR);
    // проверяем что теперь все хорошо и картинка трехканальная (но при этом каждый цвет - 32-битное вещественное число)
    rassert(houghSpaceWithCrosses.type() == CV_32FC3, 347823472890148);

    for (int i = 0; i < lines.size(); ++i) {
        PolarLineExtremum line = lines[i];

        // Пример как рисовать кружок в какой-то точке (закомментируйте его):
//        cv::Point point(100, 50);
//        cv::Scalar color(0, 0, 255); // BGR, т.е. красный цвет
//        cv::circle(houghSpaceWithCrosses, point, 3, color);

        // TODO отметьте в пространстве Хафа красным кружком радиуса radius экстремум соответствующий прямой line
        cv::Point point(line.theta, line.r);
        cv::Scalar color(0, 0, 255); // BGR, т.е. красный цвет
        cv::circle(houghSpaceWithCrosses, point, radius, color);
    }

    return houghSpaceWithCrosses;
}

cv::Point PolarLineExtremum::intersect(PolarLineExtremum that)
{
    // Одна прямая - наш текущий объект (this) у которого был вызван этот метод, у этой прямой такие параметры:
    double theta0 = this->theta;
    double r0 = this->r;

    int a1 = cos(toRadians(theta0));
    int b1 = sin(toRadians(theta0));
    int c1 = -r0;
    // Другая прямая - другой объект (that) который был передан в этот метод как аргумент, у этой прямой такие параметры:
    double theta1 = that.theta;
    double r1 = that.r;

    int a2 = cos(toRadians(theta1));
    int b2 = sin(toRadians(theta1));
    int c2 = -r1;
    // TODO реализуйте поиск пересечения этих двух прямых, напоминаю что формула прямой описана тут - https://www.polarnick.com/blogs/239/2021/school239_11_2021_2022/2021/11/02/lesson8-hough-transform.html
    // после этого загуглите как искать пересечение двух прямых, пример запроса: "intersect two 2d lines"
    // и не забудьте что cos/sin принимают радианы (используйте toRadians)

    const double EPS = 1e-3;
    double zn = det (a1, b1, a2, b2);
    int x = - det (c1, b1, c2, b2) / zn;
    int y = - det (a1, c1, a2, c2) / zn;

    return cv::Point(x, y);
}

// TODO Реализуйте эту функцию - пусть она скопирует картинку и отметит на ней прямые в соответствии со списком прямых
cv::Mat drawLinesOnImage(cv::Mat img, std::vector<PolarLineExtremum> lines)
{
    // делаем копию картинки (чтобы при рисовании не менять саму оригинальную картинку)
    cv::Mat imgWithLines = img.clone();

    // проверяем что картинка черно-белая (мы ведь ее такой сделали ради оператора Собеля) и 8-битная
    rassert(imgWithLines.type() == CV_8UC1, 45728934700167);

    // но мы хотим рисовать КРАСНЫЕ прямые, а значит нам не подходит черно-белая картинка
    // поэтому ее надо преобразовать в обычную цветную BGR картинку с 8 битами в каждом пикселе
    cv::cvtColor(imgWithLines, imgWithLines, cv::COLOR_GRAY2BGR);
    rassert(imgWithLines.type() == CV_8UC3, 45728934700172);

    // выпишем размер картинки
    int width = imgWithLines.cols;
    int height = imgWithLines.rows;

    for (int i = 0; i < lines.size(); ++i) {
        PolarLineExtremum line = lines[i];

        // нам надо найти точки на краях картинки
        cv::Point pointA;
        cv::Point pointB;

        // TODO создайте четыре прямых соответствующих краям картинки (на бумажке нарисуйте картинку и подумайте какие theta/r должны быть у прямых?):
        // напоминаю - чтобы посмотреть какие аргументы требует функция (или в данном случае конструктор объекта) - нужно:
        // 1) раскомментировать эти четыре строки ниже
        // 2) поставить каретку (указатель где вы вводите новые символы) внутри скобок функции (или конструктора, т.е. там где были три вопроса: ???)
        // 3) нажать Ctrl+P чтобы показать список параметров (P=Parameters)
        PolarLineExtremum leftImageBorder(0, 0, 0);
        PolarLineExtremum bottomImageBorder(90, height, 0);
        PolarLineExtremum rightImageBorder(0, width, 0);
        PolarLineExtremum topImageBorder(90, 0, 0);

        // TODO воспользуйтесь недавно созданной функций поиска пересечения прямых чтобы найти точки пересечения краев картинки:
        pointA = line.intersect(bottomImageBorder);
        pointB = line.intersect(topImageBorder);

        // TODO а в каких случаях нужно использовать пересечение с верхним и нижним краем картинки?
        if(!(line.theta < 10 || line.theta > 350 || (line.theta < 190 && line.theta > 170)))
        {

            pointA = line.intersect(leftImageBorder);
            pointB = line.intersect(rightImageBorder);
        }


        // TODO переделайте так чтобы цвет для каждой прямой был случайным (чтобы легче было различать близко расположенные прямые)
        cv::Scalar color(rand() % 255, rand() % 255, rand() % 255);
        cv::line(imgWithLines, pointA, pointB, color);
    }

    return imgWithLines;
}
