#include <vector>
#include <iostream>
#include <libutils/rasserts.h>
#include "helper_function.h" // подключаем функцию print2DArray(...) из соседнего файла


void task1() {
    std::vector<int> a; // динамический массив (автоматически увеличивается по мере добавления объектов)
    a.push_back(10); // добавляем в конец число 10, тем самым увеличивая этот динамический массив
    std::cout << "current size is " << a.size() << std::endl; // печатаем в консоль текущий размер

    // чтобы продолжить выполнение задания - раскомментируйте следующие строчки - проще всего это сделать выделив их все мышкой, а затем нажать Ctrl+/ (так же можно и комментировать строки)
    rassert(a.size() == 1,
            3412421421); // это способ проверить некоторый факт в программе, например здесь мы ожидали бы что размер вектора 1 - давайте явно это проверим, указав код ошибки на случай если это будет не так
    rassert(a.size() == 1,
            "vector a should of size 3!"); // можно вместо числа писать текст описывающий проблему, но на мой взгляд это слишком долго и лениво

    a.resize(5); // изменяем размер массива, пусть он будет хранить пять чисел
    for (int i = 0; i < 5; ++i) {
        a[i] = i; // тогда мы сможем сразу в каждую из его пяти ячеек что-то положить, например числа от 0 до 4
    }
    std::cout << "current size is " << a.size() << std::endl; // печатаем в консоль текущий размер
    rassert(a.size() == 5, 3412421421);
}
void task2() {
    std::vector<std::vector<int>> array2d; // это динамический массив в котором каждый элемент - тоже динамический массив

    rassert(array2d.size()==0, 12395641);
    std::vector<int> row0; // первый ряд чисел для нашего двумерного массива (row = ряд)
    row0.push_back(5);
    row0.push_back(8);
    row0.push_back(10);
    rassert(row0.size()==3, 1285642641);

    array2d.push_back(row0);
    rassert(array2d.size()==1, 1256348761);


    std::cout << "print2DArray(array2d):" << std::endl;
    print2DArray(array2d);
    // чтобы продолжить выполнение задания - раскомментируйте следующие строчки - проще всего это сделать выделив их все мышкой, а затем нажать Ctrl+/ (так же можно и комментировать строки)
    // теперь давайте добавим еще один ряд чисел но другим способом:
    std::vector<int> row1;
    row1.resize(3);
    row1[0] = 23;
    row1[1] = 24;
    row1[2] = 42;
    array2d.push_back(row1);
    rassert(row1.size() == 3, 6378943265);
    rassert(array2d.size() == 2, 7962032);
    std::cout << "print2DArray(array2d):" << std::endl;
    print2DArray(array2d);

    std::vector<int> row3;
    for(int i = 0; i < 10; i++){
        row3.push_back(i);
    }
    array2d.push_back(row3);
    std::cout << "print2DArray(array2d):" << std::endl;
    print2DArray(array2d);
}

void task3() {

    int rows, cols;
    std::cin >> rows >> cols;
    rassert(rows > 0 && rows < 21, "rows is out of range");
    rassert(cols > 0 && cols < 21, "cols is out of range");
    std::vector< std::vector<int> > array2d(rows, std::vector<int> (cols));

    while (true) {
        int i;
        int j;
        std::cin >> i >> j;
        if (i < 0 || j < 0)
            break;
        array2d[i][j]++;
        print2DArray(array2d);

        for(int k = 0; k < rows; k++){
            bool f = true;
            for(int d = 0; d < cols; d++){
                if(array2d[k][d] != 1)
                    f = false;
            }
            if ( f == true )
            {
                std::cout << "OX-XO-XO" << '\n';
                break;
            }
        }

        for(int k = 0; k < cols; k++){
            bool f = true;
            for(int d = 0; d < rows; d++){
                if(array2d[d][k] != 1)
                    f = false;
            }
            if ( f == true )
            {
                std::cout << "AX-XA-XA" << '\n';
                break;
            }
        }

    }

}


int main() {
    try {
  //    task1();
        task2();
//      task3();
        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}