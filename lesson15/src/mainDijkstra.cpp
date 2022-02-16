#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <queue>
using namespace std;

int debugPoint(int line) {
    if (line < 0)
        return 0;

    // You can put breakpoint at the following line to catch any rassert failure:
    return line;
}

#define rassert(condition, message) if (!(condition)) { std::stringstream ss; (ss << "Assertion \"" << message << "\" failed at line " << debugPoint(__LINE__) << "!"); throw std::runtime_error(ss.str()); }


struct Edge {
    int u, v; // номера вершин которые это ребро соединяет
    int w; // длина ребра (т.е. насколько длинный путь предстоит преодолеть переходя по этому ребру между вершинами)

    Edge(int u, int v, int w) : u(u), v(v), w(w)
    {}
};

void run() {
    // https://codeforces.com/problemset/problem/20/C?locale=ru
    // Не требуется сделать оптимально быструю версию, поэтому если вы получили:
    //
    // Превышено ограничение времени на тесте 31
    //
    // То все замечательно и вы молодец.

    int n, m;
    std::cin >> n;
    std::cin >> m;

    std::vector<std::vector<Edge>> g(n);
    for (int i = 0; i < m; ++i) {
        int ai, bi, w;
        std::cin >> ai >> bi >> w;

        ai -= 1;
        bi -= 1;

        g[ai].push_back(Edge(ai, bi, w));

        g[bi].push_back(Edge(bi, ai, w)); // а тут - обратное ребро, можно конструировать объект прямо в той же строчке где он и потребовался
    }

    const int s = 0;
    const int f = n - 1;

    const int INF = std::numeric_limits<int>::max();

    vector<int> d (n, INF),  p (n);
    d[s] = 0;
    priority_queue < pair<int,int> > q;
    q.push (make_pair (0, s));
    while (!q.empty()) {
        int v = q.top().second,  cur_d = -q.top().first;
        q.pop();
        if (cur_d > d[v])  continue;

        for (size_t j=0; j<g[v].size(); ++j) {
            int to = g[v][j].v,
            len = g[v][j].w;
            if (d[v] + len < d[to]) {
                d[to] = d[v] + len;
                p[to] = v;
                q.push (make_pair (-d[to], to));
            }
        }
    }

    if(d[f] == INF) {
        cout << -1;
    }
    else{
        vector<int> path;
        for (int v=f; v!=s; v=p[v])
            path.push_back (v);
        path.push_back (s);
        for(int i = path.size() - 1; i >= 0; i--){
            cout << path[i] + 1 << " ";
        }
    }


}

int main() {
    try {
        run();

        return 0;
    } catch (const std::exception &e) {
        std::cout << "Exception! " << e.what() << std::endl;
        return 1;
    }
}
