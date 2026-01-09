#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// 数独格子结构体
typedef struct {
    int value;          // 0表示未知，1-9表示已确定取值
    int maybe[9];       // 该格子1-9是否有可能，0表示可能，1表示不可能
    int row;            // 行索引
    int col;            // 列索引
    int box;            // 3x3宫索引
    int is_fixed;       // 是否为初始固定值
} SudokuCell;

// 数独游戏结构体
typedef struct {
    SudokuCell grid[9][9];  // 9x9网格
    int fixed_count;        // 固定格子数量
    int solved;             // 已解决的格子数
    int total_unknown;      // 初始未知格子数
    int backtracks;         // 回溯次数
    int steps;              // 尝试步骤数
} SudokuGame;

// 全局示例数独（可选）
int example_sudoku[9][9] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
};

// 空数独示例
int empty_sudoku[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// 困难数独示例
int hard_sudoku1[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 3, 0, 8, 5},
    {0, 0, 1, 0, 2, 0, 0, 0, 0},
    {0, 0, 0, 5, 0, 7, 0, 0, 0},
    {0, 0, 4, 0, 0, 0, 1, 0, 0},
    {0, 9, 0, 0, 0, 0, 0, 0, 0},
    {5, 0, 0, 0, 0, 0, 0, 7, 3},
    {0, 0, 2, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 4, 0, 0, 0, 9}
};

int hard_sudoku[9][9] = {
    {9, 0, 0, 0, 2, 0, 0, 3, 1},
    {6, 0, 0, 0, 8, 0, 0, 5, 0},
    {0, 0, 0, 3, 0, 1, 8, 0, 0},
    {0, 2, 8, 0, 5, 6, 7, 0, 0},
    {5, 0, 3, 0, 0, 0, 0, 0, 8},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 6, 0, 0, 5, 1, 2, 0},
    {0, 5, 0, 6, 0, 0, 0, 0, 0},
    {0, 0, 9, 0, 7, 0, 0, 0, 0}
};


// 初始化数独游戏
void init_sudoku(SudokuGame *game, int input_grid[9][9]) {
    game->fixed_count = 0;
    game->solved = 0;
    game->total_unknown = 0;
    game->backtracks = 0;
    game->steps = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            game->grid[i][j].value = input_grid[i][j];
            game->grid[i][j].row = i;
            game->grid[i][j].col = j;
            game->grid[i][j].box = (i / 3) * 3 + (j / 3);
            
            // 标记是否为固定值
            game->grid[i][j].is_fixed = (input_grid[i][j] > 0);
            
            // 初始化maybe数组
            for (int k = 0; k < 9; k++) {
                game->grid[i][j].maybe[k] = 0;  // 0表示可能
            }
            
            if (input_grid[i][j] == 0) {
                game->total_unknown++;
            } else {
                game->solved++;
                game->fixed_count++;
            }
        }
    }
}

// 检查数字是否可放置在指定位置
int is_safe(SudokuGame *game, int row, int col, int num) {
    // 检查行
    for (int c = 0; c < 9; c++) {
        if (game->grid[row][c].value == num) {
            return 0;  // 冲突
        }
    }
    
    // 检查列
    for (int r = 0; r < 9; r++) {
        if (game->grid[r][col].value == num) {
            return 0;  // 冲突
        }
    }
    
    // 检查3x3宫
    int box_row = (row / 3) * 3;
    int box_col = (col / 3) * 3;
    for (int r = box_row; r < box_row + 3; r++) {
        for (int c = box_col; c < box_col + 3; c++) {
            if (game->grid[r][c].value == num) {
                return 0;  // 冲突
            }
        }
    }
    
    return 1;  // 安全
}

// 计算一个格子的可能数字数量
int count_possibilities(SudokuGame *game, int row, int col) {
    int count = 0;
    for (int num = 1; num <= 9; num++) {
        if (is_safe(game, row, col, num)) {
            count++;
        }
    }
    return count;
}

// 找到具有最少可能性的格子
int find_best_cell(SudokuGame *game, int *row, int *col) {
    int min_possibilities = 10;  // 初始化为大于9的值
    *row = -1;
    *col = -1;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                int possibilities = count_possibilities(game, i, j);
                if (possibilities < min_possibilities) {
                    min_possibilities = possibilities;
                    *row = i;
                    *col = j;
                }
            }
        }
    }
    
    return (*row != -1 && *col != -1);
}

// 获取一个格子的所有可能数字
void get_possible_numbers(SudokuGame *game, int row, int col, int possible[], int *count) {
    *count = 0;
    for (int num = 1; num <= 9; num++) {
        if (is_safe(game, row, col, num)) {
            possible[*count] = num;
            (*count)++;
        }
    }
}

// 数独求解的回溯函数
int solve_backtrack(SudokuGame *game) {
    game->steps++;
    
    // 检查是否所有格子都已填充
    int row = -1, col = -1;
    
    // 使用MRV（最小剩余值）启发式：找到可能性最少的格子
    if (!find_best_cell(game, &row, &col)) {
        return 1;  // 数独已解决
    }
    
    // 尝试所有可能的数字
    int possible[9];
    int possible_count;
    get_possible_numbers(game, row, col, possible, &possible_count);
    
    for (int i = 0; i < possible_count; i++) {
        int num = possible[i];
        
        // 放置数字
        game->grid[row][col].value = num;
        game->solved++;
        
        // 递归解决剩余部分
        if (solve_backtrack(game)) {
            return 1;  // 找到解
        }
        
        // 回溯：撤销选择
        game->grid[row][col].value = 0;
        game->solved--;
        game->backtracks++;
    }
    
    return 0;  // 无解
}

// 刷新所有格子的可能性数据（用于显示）
void refresh_possibilities(SudokuGame *game) {
    // 重置所有格子的可能性
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                for (int k = 0; k < 9; k++) {
                    game->grid[i][j].maybe[k] = 0;  // 重置为可能
                }
            }
        }
    }
    
    // 根据已确定的数字更新不可能性
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            int value = game->grid[i][j].value;
            if (value > 0) {
                int num = value - 1;  // 转换为0-based索引
                int box_row = (i / 3) * 3;
                int box_col = (j / 3) * 3;
                
                // 更新同一行
                for (int c = 0; c < 9; c++) {
                    if (game->grid[i][c].value == 0) {
                        game->grid[i][c].maybe[num] = 1;  // 1表示不可能
                    }
                }
                
                // 更新同一列
                for (int r = 0; r < 9; r++) {
                    if (game->grid[r][j].value == 0) {
                        game->grid[r][j].maybe[num] = 1;  // 1表示不可能
                    }
                }
                
                // 更新同一3x3宫
                for (int r = box_row; r < box_row + 3; r++) {
                    for (int c = box_col; c < box_col + 3; c++) {
                        if (game->grid[r][c].value == 0) {
                            game->grid[r][c].maybe[num] = 1;  // 1表示不可能
                        }
                    }
                }
            }
        }
    }
}

// 检查数独是否完整
int check_complete(SudokuGame *game) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                return 0;  // 未完成
            }
        }
    }
    return 1;  // 已完成
}

// 打印数独
void print_sudoku(SudokuGame *game) {
    printf("\n数独结果:\n");
    printf("+-------+-------+-------+\n");
    
    for (int i = 0; i < 9; i++) {
        printf("| ");
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                printf(". ");
            } else {
                if (game->grid[i][j].is_fixed) {
                    printf("\033[1;32m%d\033[0m ", game->grid[i][j].value);  // 绿色显示固定值
                } else {
                    printf("%d ", game->grid[i][j].value);  // 普通显示求解值
                }
            }
            
            if (j % 3 == 2) {
                printf("| ");
            }
        }
        printf("\n");
        
        if (i % 3 == 2) {
            printf("+-------+-------+-------+\n");
        }
    }
}

// 打印可能性统计（调试用）
void print_possibilities(SudokuGame *game) {
    printf("\n各格子可能性统计:\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                int count = 0;
                for (int k = 0; k < 9; k++) {
                    if (game->grid[i][j].maybe[k] == 0) {
                        count++;
                    }
                }
                printf("(%d,%d): %d种可能  ", i, j, count);
                if (j % 3 == 2) printf(" ");
            } else {
                printf("(%d,%d): 已确定   ", i, j);
                if (j % 3 == 2) printf(" ");
            }
        }
        printf("\n");
        if (i % 3 == 2) printf("\n");
    }
}

// 从命令行读取数独
void read_sudoku_from_cmdline(int grid[9][9]) {
    printf("请输入数独（9行，每行9个数字，0表示未知，用空格或逗号分隔）:\n");
    
    for (int i = 0; i < 9; i++) {
        printf("第%d行: ", i + 1);
        
        char line[256];
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        int col = 0;
        char *token = strtok(line, " ,\t\n");
        
        while (token != NULL && col < 9) {
            if (isdigit(token[0])) {
                grid[i][col] = atoi(token);
                if (grid[i][col] < 0 || grid[i][col] > 9) {
                    grid[i][col] = 0;
                }
                col++;
            }
            token = strtok(NULL, " ,\t\n");
        }
        
        // 如果输入不足9个，用0填充
        while (col < 9) {
            grid[i][col] = 0;
            col++;
        }
    }
}

// 验证数独解是否正确
int validate_solution(SudokuGame *game) {
    // 检查所有行
    for (int i = 0; i < 9; i++) {
        int used[10] = {0};
        for (int j = 0; j < 9; j++) {
            int val = game->grid[i][j].value;
            if (val < 1 || val > 9 || used[val]) {
                return 0;  // 无效
            }
            used[val] = 1;
        }
    }
    
    // 检查所有列
    for (int j = 0; j < 9; j++) {
        int used[10] = {0};
        for (int i = 0; i < 9; i++) {
            int val = game->grid[i][j].value;
            if (val < 1 || val > 9 || used[val]) {
                return 0;  // 无效
            }
            used[val] = 1;
        }
    }
    
    // 检查所有3x3宫
    for (int box = 0; box < 9; box++) {
        int used[10] = {0};
        int box_row = (box / 3) * 3;
        int box_col = (box % 3) * 3;
        for (int i = box_row; i < box_row + 3; i++) {
            for (int j = box_col; j < box_col + 3; j++) {
                int val = game->grid[i][j].value;
                if (val < 1 || val > 9 || used[val]) {
                    return 0;  // 无效
                }
                used[val] = 1;
            }
        }
    }
    
    return 1;  // 有效
}

// 主求解函数（使用回溯法）
int solve_sudoku_backtrack(SudokuGame *game) {
    clock_t start_time = clock();
    
    printf("开始回溯求解...\n");
    
    // 刷新可能性数据（用于显示）
    refresh_possibilities(game);
    
    // 使用回溯法求解
    int solved = solve_backtrack(game);
    
    clock_t end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\n求解统计:\n");
    printf("求解时间: %.6f 秒\n", elapsed_time);
    printf("尝试步骤数: %d\n", game->steps);
    printf("回溯次数: %d\n", game->backtracks);
    
    return solved;
}

// 打印初始数独
void print_initial_sudoku(int grid[9][9]) {
    printf("\n初始数独:");
    printf("\n+-------+-------+-------+\n");
    
    for (int i = 0; i < 9; i++) {
        printf("| ");
        for (int j = 0; j < 9; j++) {
            if (grid[i][j] == 0) {
                printf(". ");
            } else {
                printf("%d ", grid[i][j]);
            }
            
            if (j % 3 == 2) {
                printf("| ");
            }
        }
        printf("\n");
        
        if (i % 3 == 2) {
            printf("+-------+-------+-------+\n");
        }
    }
}

// 主函数
int main(int argc, char *argv[]) {
    int grid[9][9];
    int use_example = 0;
    int use_empty = 0;
    int use_hard = 0;
    
    printf("=== 数独求解器（回溯法实现） ===\n\n");
    
    // 解析命令行参数
    if (argc > 1) {
        if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--example") == 0) {
            use_example = 1;
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("使用说明:\n");
            printf("  %s -e    使用示例数独\n", argv[0]);
            printf("  %s -d    使用困难数独\n", argv[0]);
            printf("  %s -c    从命令行输入数独\n", argv[0]);
            printf("  %s       默认从命令行输入\n", argv[0]);
            return 0;
        } else if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--difficult") == 0) {
            use_hard = 1;
        } else if (strcmp(argv[1], "-empty") == 0) {
            use_empty = 1;
        }
    }
    
    // 获取数独数据
    if (use_example) {
        printf("使用示例数独...\n");
        memcpy(grid, example_sudoku, sizeof(example_sudoku));
    } else if (use_hard) {
        printf("使用困难数独...\n");
        memcpy(grid, hard_sudoku, sizeof(hard_sudoku));
    } else if (use_empty) {
        printf("使用空数独...\n");
        memcpy(grid, empty_sudoku, sizeof(empty_sudoku));
    } else {
        printf("请选择输入方式:\n");
        printf("1. 手动输入数独\n");
        printf("2. 使用示例数独\n");
        printf("3. 使用困难数独\n");
        printf("4. 使用空数独\n");
        printf("请选择 (1-4): ");
        
        char choice[10];
        fgets(choice, sizeof(choice), stdin);
        
        switch (choice[0]) {
            case '2':
                memcpy(grid, example_sudoku, sizeof(example_sudoku));
                break;
            case '3':
                memcpy(grid, hard_sudoku, sizeof(hard_sudoku));
                break;
            case '4':
                memcpy(grid, empty_sudoku, sizeof(empty_sudoku));
                break;
            case '1':
            default:
                read_sudoku_from_cmdline(grid);
                break;
        }
    }
    
    // 打印初始数独
    print_initial_sudoku(grid);
    
    // 创建和初始化数独游戏
    SudokuGame game;
    init_sudoku(&game, grid);
    
    printf("\n初始状态: %d 个已知数字，%d 个未知格子\n", 
           game.fixed_count, game.total_unknown);
    
    // 求解数独
    int solved = solve_sudoku_backtrack(&game);
    
    // 刷新可能性数据用于显示
    refresh_possibilities(&game);
    
    if (solved) {
        printf("\n数独求解成功！\n");
        print_sudoku(&game);
        printf("\n总共解决了 %d 个未知格子\n", game.solved - game.fixed_count);
        
        // 验证解是否正确
        if (validate_solution(&game)) {
            printf("解验证: 正确 ✓\n");
        } else {
            printf("解验证: 错误 ✗\n");
        }
    } else {
        printf("\n数独求解失败！\n");
        print_sudoku(&game);
        printf("\n已解决 %d/%d 个格子\n", game.solved - game.fixed_count, game.total_unknown);
        
        // 显示剩余格子的可能性
        print_possibilities(&game);
    }
    
    return 0;
}

