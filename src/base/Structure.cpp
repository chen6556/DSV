#include "base/Structure.hpp"


// 负数权重表示不连通
namespace DSGraph
{
    void show_mat(const std::vector<std::vector<int>>& mat)
    {
        int temp = INT_MIN;
        for (const std::vector<int>& nums : mat)
        {
            temp = std::max(temp, *std::max_element(nums.cbegin(), nums.cend()));
        }
        const size_t count = std::max(mat.size(), std::max_element(mat.cbegin(), mat.cend(), [](const std::vector<int>& row0, const std::vector<int>& row1){return row0.size() < row1.size();})->size());
        const size_t n = std::max(static_cast<size_t>(2), std::to_string(temp).size());
        const std::string line(n * mat.front().size() + count, '-'), space(n, ' '); 
        std::cout << std::endl << line << std::endl;
        for (const std::vector<int>& row : mat)
        {
            for (const int& i : row)
            {
                std::cout << space.substr(0, n - std::to_string(i).size()) << i << ' ' ;
            }
            std::cout << std::endl;
        }
        std::cout << line << std::endl;
    }

    const std::vector<size_t> dfs(const std::vector<std::vector<int>>& mat, const size_t& start)
    {
        const size_t count = mat.size();
        std::vector<size_t> marks;
        std::stack<size_t> stack;
        size_t temp;
        stack.push(start);
        AVLTree<size_t> tree;
        while (!stack.empty())
        {
            temp = stack.top();
            stack.pop();
            if (tree.append(temp))
            {
                marks.push_back(temp);
            }
            for (size_t i = 1; i <= count; ++i)
            {
                if (mat[temp][count - i] > 0 && !tree.has(count - i))
                {
                    stack.push(count - i);
                }
            }
        }
        return marks;
    }

    const std::vector<size_t> bfs(const std::vector<std::vector<int>>& mat, const size_t& start)
    {
        const size_t count = mat.size();
        std::vector<size_t> marks = {start};
        std::list<size_t> next_lay, lay = {start};
        AVLTree<size_t> tree({start});
        while (!lay.empty())
        {
            for (const size_t& j : lay)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    if (mat[j][i] > 0 && tree.append(i))
                    {
                        marks.push_back(i);
                        next_lay.push_back(i);
                    }
                }
            }
            lay.assign(next_lay.cbegin(), next_lay.cend());
            next_lay.clear();
        }
        return marks;
    }

    const std::vector<std::vector<int>> prim(const std::vector<std::vector<int>>& mat)
    {
        const size_t count = mat.size();
        std::vector<std::vector<int>> result(count, std::vector<int>(count, -1));
        std::vector<int> low_cost(mat.front());
        std::vector<size_t> adjvex(count, 0);
        int min_cost;
        for (size_t k, i = 1; i < count; ++i)
        {
            min_cost = INT_MAX;
            k = 0;
            for (size_t j = 1; j < count; ++j)
            {
                if (low_cost[j] > 0 && low_cost[j] < min_cost) // 负值表示不连通
                {
                    min_cost = low_cost[j];
                    k = j;
                }
            }
            result[adjvex[k]][k] = min_cost;
            low_cost[k] = 0;

            for (size_t j = 1; j < count; ++j)
            {
                if (low_cost[j] != 0 && mat[k][j] > 0 && (mat[k][j] < low_cost[j] || low_cost[j] < 0)) // 负值表示不连通
                {
                    low_cost[j] = mat[k][j];
                    adjvex[j] = k;
                }
            }
        }
        return result;
    }

    const std::vector<std::vector<size_t>> dijkstra(const std::vector<std::vector<int>>& mat, const size_t& start)
    {
        const size_t count = mat.size();
        std::list<size_t> nopassed;
        std::vector<int> weights(mat[start]);
        std::vector<std::vector<size_t>> result(count, std::vector<size_t>({start}));
        for (size_t i = 0; i < count; ++i)
        {
            if (i != start)
            {
                nopassed.push_back(i);
            }
        }

        size_t index;
        while (!nopassed.empty())
        {
            index = nopassed.front();
            for (const size_t& i : nopassed)
            {
                if (0 < weights[i] && weights[i] < weights[index])
                {
                    index = i;
                }
            }
            nopassed.remove(index);

            for (const size_t& i : nopassed)
            {
                if (weights[index] > 0 && mat[index][i] > 0 && weights[i] > weights[index] + mat[index][i])
                {
                    weights[i] = weights[index] + mat[index][i];
                    result[i].assign(result[index].cbegin(), result[index].cend());
                    result[i].push_back(index);
                }
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            result[i].push_back(i);
        }

        return result;
    }

    const std::vector<std::vector<size_t>> floyd(const std::vector<std::vector<int>>& mat)
    {
        const size_t count = mat.size();
        std::vector<std::vector<int>> weights(mat);
        std::vector<std::vector<size_t>> result;
        for (const std::vector<int>& row : mat)
        {
            result.push_back(std::vector<size_t>());
            for (size_t i = 0; i < count; ++i)
            {
                if (row[i] >= 0)
                {
                    result.back().push_back(i);
                }
                else
                {
                    // SIZE_MAX表示不连通
                    result.back().push_back(SIZE_MAX);
                }
            }
        }

        for (size_t n = 0; n < count; ++n)
        {
            for (size_t i = 0; i < count; ++i)
            {
                for (size_t j = 0; j < count; ++j)
                {
                    if (weights[i][n] > 0 && weights[n][j] > 0 &&
                        weights[i][j] > weights[i][n] + weights[n][j])
                    {
                        weights[i][j] = weights[i][n] + weights[n][j];
                        result[i][j] = result[i][n];
                    }
                }
            }
        }

        return result;
    }
};