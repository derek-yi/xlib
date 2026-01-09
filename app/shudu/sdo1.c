#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 数独格子结构体
typedef struct {
    int value;          // 0表示未知，1-9表示已确定取值
    int maybe[9];       // 该格子1-9是否有可能，0表示可能，1表示不可能
    int row;            // 行索引
    int col;            // 列索引
    int box;            // 3x3宫索引
} SudokuCell;

// 数独游戏结构体
typedef struct {
    SudokuCell grid[9][9];  // 9x9网格
    int solved;             // 已解决的格子数
    int total_unknown;      // 初始未知格子数
} SudokuGame;

// 全局示例数独（可选）
int example_sudoku1[9][9] = {
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

int example_sudoku[9][9] = {
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
    game->solved = 0;
    game->total_unknown = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            game->grid[i][j].value = input_grid[i][j];
            game->grid[i][j].row = i;
            game->grid[i][j].col = j;
            game->grid[i][j].box = (i / 3) * 3 + (j / 3);
            
            // 初始化maybe数组
            for (int k = 0; k < 9; k++) {
                game->grid[i][j].maybe[k] = 0;  // 0表示可能
            }
            
            if (input_grid[i][j] == 0) {
                game->total_unknown++;
            } else {
                game->solved++;
            }
        }
    }
}

// 刷新所有格子的可能性数据
void refresh_possibilities(SudokuGame *game) {
    // 首先重置所有未知格子的可能性
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

// 检查格子是否冲突
int check_conflict(SudokuGame *game, int row, int col, int value) {
    // 检查行
    for (int c = 0; c < 9; c++) {
        if (c != col && game->grid[row][c].value == value) {
            return 1;  // 冲突
        }
    }
    
    // 检查列
    for (int r = 0; r < 9; r++) {
        if (r != row && game->grid[r][col].value == value) {
            return 1;  // 冲突
        }
    }
    
    // 检查3x3宫
    int box_row = (row / 3) * 3;
    int box_col = (col / 3) * 3;
    for (int r = box_row; r < box_row + 3; r++) {
        for (int c = box_col; c < box_col + 3; c++) {
            if (r != row && c != col && game->grid[r][c].value == value) {
                return 1;  // 冲突
            }
        }
    }
    
    return 0;  // 无冲突
}

// 行试探
int row_trial(SudokuGame *game) {
    int success_count = 0;
    
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (game->grid[i][j].value == 0) {
                // 检查当前格子的可能性
                int possible_count = 0;
                int possible_value = 0;
                
                for (int k = 0; k < 9; k++) {
                    if (game->grid[i][j].maybe[k] == 0) {
                        // 检查在列或3x3宫中是否冲突
                        int conflict_in_col = 0;
                        int conflict_in_box = 0;
                        
                        // 检查列冲突
                        for (int r = 0; r < 9; r++) {
                            if (r != i && game->grid[r][j].value == k + 1) {
                                conflict_in_col = 1;
                                game->grid[i][j].maybe[k] = 1;  // 更新为不可能
                                break;
                            }
                        }
                        
                        // 检查3x3宫冲突
                        if (!conflict_in_col) {
                            int box_row = (i / 3) * 3;
                            int box_col = (j / 3) * 3;
                            for (int r = box_row; r < box_row + 3; r++) {
                                for (int c = box_col; c < box_col + 3; c++) {
                                    if (!(r == i && c == j) && game->grid[r][c].value == k + 1) {
                                        conflict_in_box = 1;
                                        game->grid[i][j].maybe[k] = 1;  // 更新为不可能
                                        break;
                                    }
                                }
                                if (conflict_in_box) break;
                            }
                        }
                        
                        // 如果没有冲突，记录可能性
                        if (!conflict_in_col && !conflict_in_box) {
                            possible_count++;
                            possible_value = k + 1;
                        }
                    }
                }
                
                // 如果只有一个可能的值
                if (possible_count == 1) {
                    game->grid[i][j].value = possible_value;
                    game->solved++;
                    success_count++;
                    
                    // 立即刷新可能性
                    refresh_possibilities(game);
                }
            }
        }
    }
    
    return success_count;
}

// 列试探
int col_trial(SudokuGame *game) {
    int success_count = 0;
    
    for (int j = 0; j < 9; j++) {
        for (int i = 0; i < 9; i++) {
            if (game->grid[i][j].value == 0) {
                // 检查当前格子的可能性
                int possible_count = 0;
                int possible_value = 0;
                
                for (int k = 0; k < 9; k++) {
                    if (game->grid[i][j].maybe[k] == 0) {
                        // 检查在行或3x3宫中是否冲突
                        int conflict_in_row = 0;
                        int conflict_in_box = 0;
                        
                        // 检查行冲突
                        for (int c = 0; c < 9; c++) {
                            if (c != j && game->grid[i][c].value == k + 1) {
                                conflict_in_row = 1;
                                game->grid[i][j].maybe[k] = 1;  // 更新为不可能
                                break;
                            }
                        }
                        
                        // 检查3x3宫冲突
                        if (!conflict_in_row) {
                            int box_row = (i / 3) * 3;
                            int box_col = (j / 3) * 3;
                            for (int r = box_row; r < box_row + 3; r++) {
                                for (int c = box_col; c < box_col + 3; c++) {
                                    if (!(r == i && c == j) && game->grid[r][c].value == k + 1) {
                                        conflict_in_box = 1;
                                        game->grid[i][j].maybe[k] = 1;  // 更新为不可能
                                        break;
                                    }
                                }
                                if (conflict_in_box) break;
                            }
                        }
                        
                        // 如果没有冲突，记录可能性
                        if (!conflict_in_row && !conflict_in_box) {
                            possible_count++;
                            possible_value = k + 1;
                        }
                    }
                }
                
                // 如果只有一个可能的值
                if (possible_count == 1) {
                    game->grid[i][j].value = possible_value;
                    game->solved++;
                    success_count++;
                    
                    // 立即刷新可能性
                    refresh_possibilities(game);
                }
            }
        }
    }
    
    return success_count;
}

// 3x3宫试探
int box_trial(SudokuGame *game) {
    int success_count = 0;
    
    // 遍历每个3x3宫
    for (int box = 0; box < 9; box++) {
        int box_row = (box / 3) * 3;
        int box_col = (box % 3) * 3;
        
        // 遍历宫内的每个格子
        for (int r = box_row; r < box_row + 3; r++) {
            for (int c = box_col; c < box_col + 3; c++) {
                if (game->grid[r][c].value == 0) {
                    // 检查当前格子的可能性
                    int possible_count = 0;
                    int possible_value = 0;
                    
                    for (int k = 0; k < 9; k++) {
                        if (game->grid[r][c].maybe[k] == 0) {
                            // 检查在行或列中是否冲突
                            int conflict_in_row = 0;
                            int conflict_in_col = 0;
                            
                            // 检查行冲突
                            for (int col = 0; col < 9; col++) {
                                if (col != c && game->grid[r][col].value == k + 1) {
                                    conflict_in_row = 1;
                                    game->grid[r][c].maybe[k] = 1;  // 更新为不可能
                                    break;
                                }
                            }
                            
                            // 检查列冲突
                            if (!conflict_in_row) {
                                for (int row = 0; row < 9; row++) {
                                    if (row != r && game->grid[row][c].value == k + 1) {
                                        conflict_in_col = 1;
                                        game->grid[r][c].maybe[k] = 1;  // 更新为不可能
                                        break;
                                    }
                                }
                            }
                            
                            // 如果没有冲突，记录可能性
                            if (!conflict_in_row && !conflict_in_col) {
                                possible_count++;
                                possible_value = k + 1;
                            }
                        }
                    }
                    
                    // 如果只有一个可能的值
                    if (possible_count == 1) {
                        game->grid[r][c].value = possible_value;
                        game->solved++;
                        success_count++;
                        
                        // 立即刷新可能性
                        refresh_possibilities(game);
                    }
                }
            }
        }
    }
    
    return success_count;
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
                printf("%d ", game->grid[i][j].value);
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
            }
        }
        printf("\n");
    }
}

// 从命令行读取数独
void read_sudoku_from_cmdline(int grid[9][9]) {
    printf("请输入数独（9行，每行9个数字，0表示未知，用空格或逗号分隔）:\n");
    
    for (int i = 0; i < 9; i++) {
        printf("第%d行: ", i + 1);
        
        char line[256];
        fgets(line, sizeof(line), stdin);
        
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

// 主求解函数
int solve_sudoku(SudokuGame *game) {
    int iterations = 0;
    int max_iterations = 100;  // 防止无限循环
    
    while (game->solved < 81 && iterations < max_iterations) {
        iterations++;
        
        // 刷新可能性数据
        refresh_possibilities(game);
        
        int success_count = 0;
        
        // 行试探
        int row_success = row_trial(game);
        success_count += row_success;
        
        // 列试探
        int col_success = col_trial(game);
        success_count += col_success;
        
        // 宫试探
        int box_success = box_trial(game);
        success_count += box_success;
        
        // 如果没有新的解决，退出循环
        if (success_count == 0) {
            break;
        }
        
        printf("迭代 %d: 解决了 %d 个格子\n", iterations, success_count);
    }
    
    return check_complete(game);
}

// 主函数
int main(int argc, char *argv[]) {
    int grid[9][9];
    int use_example = 0;
    
    // 解析命令行参数
    if (argc > 1) {
        if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--example") == 0) {
            use_example = 1;
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("使用说明:\n");
            printf("  %s -e 使用示例数独\n", argv[0]);
            printf("  %s -c 从命令行输入数独\n", argv[0]);
            printf("  %s     默认从命令行输入\n", argv[0]);
            return 0;
        }
    }
    
    // 获取数独数据
    if (use_example) {
        printf("使用示例数独...\n");
        memcpy(grid, example_sudoku, sizeof(example_sudoku));
    } else {
        read_sudoku_from_cmdline(grid);
    }
    
    // 创建和初始化数独游戏
    SudokuGame game;
    init_sudoku(&game, grid);
    
    // 打印初始数独
    printf("\n初始数独:");
    print_sudoku(&game);
    
    // 求解数独
    if (solve_sudoku(&game)) {
        printf("\n数独求解成功！\n");
        print_sudoku(&game);
        printf("总共解决了 %d 个未知格子\n", game.solved - (81 - game.total_unknown));
    } else {
        printf("\n数独求解失败！\n");
        print_sudoku(&game);
        printf("已解决 %d/%d 个格子\n", game.solved - (81 - game.total_unknown), game.total_unknown);
        
        // 显示剩余格子的可能性
        print_possibilities(&game);
    }
    
    return 0;
}

