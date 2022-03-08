#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <queue>

#include <libutils/rasserts.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>


struct Edge {
    int u, v; // номера вершин которые это ребро соединяет
    int w; // длина ребра (т.е. насколько длинный путь предстоит преодолеть переходя по этому ребру между вершинами)

    Edge(int u, int v, int w) : u(u), v(v), w(w)
    {}
};

// Эта биективная функция по координате пикселя (строчка и столбик) + размерам картинки = выдает номер вершины
int encodeVertex(int row, int column, int nrows, int ncolumns) {
    rassert(row < nrows, 348723894723980017);
    rassert(column < ncolumns, 347823974239870018);
    int vertexId = row * ncolumns + column;
    return vertexId;
}

// Эта биективная функция по номеру вершины говорит какой пиксель этой вершине соовтетствует (эта функция должна быть симметрична предыдущей!)
cv::Point2i decodeVertex(int vertexId, int nrows, int ncolumns) {

    // TODO: придумайте как найти номер строки и столбика пикселю по номеру вершины (просто поймите предыдущую функцию и эта функция не будет казаться сложной)
    int row = -1;
    int column = -1;

    row = vertexId/ncolumns;
    column = vertexId % ncolumns;

    // сверим что функция симметрично сработала:
    rassert(encodeVertex(row, column, nrows, ncolumns) == vertexId, 34782974923035);

    rassert(row < nrows, 34723894720027);
    rassert(column < ncolumns, 3824598237592030);
    return cv::Point2i(column, row);
}

void run(int mazeNumber) {
    cv::Mat maze = cv::imread("C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson15\\data\\mazesImages\\maze" + std::to_string(mazeNumber) + ".png");
    rassert(!maze.empty(), 324783479230019);
    rassert(maze.type() == CV_8UC3, 3447928472389020);
    std::cout << "Maze resolution: " << maze.cols << "x" << maze.rows << std::endl;

    int nvertices = maze.cols * maze.rows; // TODO

    std::vector<std::vector<Edge>> edges_by_vertex(nvertices+5);
    for (int j = 0; j < maze.rows; ++j) {
        for (int i = 0; i < maze.cols; ++i) {
            cv::Vec3b color = maze.at<cv::Vec3b>(j, i);
            unsigned char blue = color[0];
            unsigned char green = color[1];
            unsigned char red = color[2];

            int w = (int)(blue + green + red);

            // TODO добавьте соотвтетсвующие этому пикселю ребра
            int k = encodeVertex(j, i, maze.rows, maze.cols);
            if(j == 0){
                if(i == 0){
                    edges_by_vertex[k].push_back(Edge(0,1,w));
                    edges_by_vertex[k].push_back(Edge(0,maze.cols,w));
                }
                else if(i == maze.cols - 1){
                    edges_by_vertex[k].push_back(Edge(maze.cols - 1,maze.cols - 2,w));
                    edges_by_vertex[k].push_back(Edge(maze.cols - 1,2*maze.cols - 1,w));
                }
                else{
                    edges_by_vertex[k].push_back(Edge(k,k+1,w));
                    edges_by_vertex[k].push_back(Edge(k,k-1,w));
                    edges_by_vertex[k].push_back(Edge(k,k+maze.cols,w));
                }
            }
            else if(j == maze.rows){
                if(i == 0){
                    edges_by_vertex[k].push_back(Edge(k,k+1,w));
                    edges_by_vertex[k].push_back(Edge(k,k-maze.cols,w));
                }
                else if(i == maze.cols - 1){
                    edges_by_vertex[k].push_back(Edge(k,k-1,w));
                    edges_by_vertex[k].push_back(Edge(k,k - maze.cols,w));
                }
                else{
                    edges_by_vertex[k].push_back(Edge(k,k+1,w));
                    edges_by_vertex[k].push_back(Edge(k,k-1,w));
                    edges_by_vertex[k].push_back(Edge(k,k-maze.cols,w));
                }
            }
            else{
                edges_by_vertex[k].push_back(Edge(k,k+1,w));
                edges_by_vertex[k].push_back(Edge(k,k-1,w));
                edges_by_vertex[k].push_back(Edge(k,k-maze.cols,w));
                edges_by_vertex[k].push_back(Edge(k,k+maze.cols,w));
            }
        }
    }

    int start, finish;
    if (mazeNumber >= 1 && mazeNumber <= 3) { // Первые три лабиринта очень похожи но кое чем отличаются...
        start = encodeVertex(300, 300, maze.rows, maze.cols);
        finish = encodeVertex(0, 305, maze.rows, maze.cols);
    } else if (mazeNumber == 4) {
        start = encodeVertex(154, 312, maze.rows, maze.cols);
        finish = encodeVertex(477, 312, maze.rows, maze.cols);
    } else if (mazeNumber == 5) { // Лабиринт в большом разрешении, добровольный (на случай если вы реализовали быструю Дейкстру с приоритетной очередью)
        start = encodeVertex(1200, 1200, maze.rows, maze.cols);
        finish = encodeVertex(1200, 1200, maze.rows, maze.cols);
    } else {
        rassert(false, 324289347238920081);
    }

    const long long INF = std::numeric_limits<long long>::max();

    cv::Mat window = maze.clone(); // на этой картинке будем визуализировать до куда сейчас дошла прокладка маршрута

    std::vector<long long> distances(nvertices, INF);
    std::vector<long long> d (nvertices, INF),  p (nvertices);
    d[start] = 0;
    std::priority_queue < std::pair<long long,long long> > q;
    q.push (std::make_pair (0, start));
    while (!q.empty()) {
        long long v = q.top().second,  cur_d = -q.top().first;
        q.pop();
        if (cur_d > d[v])  continue;

        for (int j=0; j<edges_by_vertex[v].size(); ++j) {
            int to = edges_by_vertex[v][j].v,
            len = edges_by_vertex[v][j].w;
            if (d[v] + len < d[to]) {
                d[to] = d[v] + len;
                p[to] = v;
                q.push (std::make_pair (-d[to], to));
            }
        }
    }

    int cnt = 0;
    for (int v=finish; v!=start; v=p[v]){
        cv::Point2i p = decodeVertex(v, maze.rows, maze.cols);
        window.at<cv::Vec3b>(p.y, p.x) = cv::Vec3b(0, 255, 0);
        cnt++;
        if(cnt == 100)
        {
            cv::imshow("Maze", window);
            cv::waitKey(1);
            cnt = 0;
        }
    }

    // TODO в момент когда вершина становится обработанной - красьте ее на картинке window в зеленый цвет и показывайте картинку:
    //    cv::Point2i p = decodeVertex(the_chosen_one, maze.rows, maze.cols);
    //    window.at<cv::Vec3b>(p.y, p.x) = cv::Vec3b(0, 255, 0);
    //    cv::imshow("Maze", window);
    //    cv::waitKey(1);
    // TODO это может тормозить, в таком случае показывайте window только после обработки каждой сотой вершины

    // TODO обозначьте найденный маршрут красными пикселями

    // TODO сохраните картинку window на диск
    cv::imwrite("C:\\Users\\vanar\\CLionProjects\\CPPExercises2021\\lesson15\\data\\mazesImages\\maze" + std::to_string(mazeNumber) + ".png", window);
    std::cout << "Finished!" << std::endl;

    // Показываем результат пока пользователь не насладиться до конца и не нажмет Escape
    while (cv::waitKey(10) != 27) {
        cv::imshow("Maze", window);
    }
}

int main() {
    try {
        int mazeNumber = 2;
        run(mazeNumber);

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}
