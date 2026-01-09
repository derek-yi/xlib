import tkinter as tk
from tkinter import messagebox
import random
import numpy as np

class Game2048:
    def __init__(self, root):
        self.root = root
        self.root.title("2048游戏 - 10x10版本")
        self.root.geometry("850x950")  # 增大窗口以适应更大的方块
        self.root.resizable(False, False)
        
        # 初始化游戏
        self.grid_size = 10  # 10x10网格
        self.cells = np.zeros((self.grid_size, self.grid_size), dtype=int)
        self.score = 0
        self.game_over = False
        
        # 颜色配置 - 扩展更多颜色以适应更大的数字
        self.colors = {
            0: "#cdc1b4",
            2: "#eee4da",
            4: "#ede0c8",
            8: "#f2b179",
            16: "#f59563",
            32: "#f67c5f",
            64: "#f65e3b",
            128: "#edcf72",
            256: "#edcc61",
            512: "#edc850",
            1024: "#edc53f",
            2048: "#edc22e",
            4096: "#ffd700",
            8192: "#ffcc00",
            16384: "#ffb300",
            32768: "#ff9900",
            65536: "#ff6600",
            131072: "#ff3300",
            262144: "#cc0000",
            524288: "#990000",
            1048576: "#660000",
            2097152: "#330000",
            4194304: "#000000"
        }
        
        # 字体颜色
        self.font_colors = {
            2: "#776e65",
            4: "#776e65",
            8: "#f9f6f2",
            16: "#f9f6f2",
            32: "#f9f6f2",
            64: "#f9f6f2",
            128: "#f9f6f2",
            256: "#f9f6f2",
            512: "#f9f6f2",
            1024: "#f9f6f2",
            2048: "#f9f6f2",
            4096: "#f9f6f2",
            8192: "#f9f6f2",
            16384: "#f9f6f2",
            32768: "#f9f6f2",
            65536: "#f9f6f2",
            131072: "#f9f6f2",
            262144: "#f9f6f2",
            524288: "#f9f6f2",
            1048576: "#f9f6f2",
            2097152: "#f9f6f2",
            4194304: "#ffffff"
        }
        
        self.setup_ui()
        self.start_game()
        
        # 绑定键盘事件
        self.root.bind("<Key>", self.on_key_press)
        
    def setup_ui(self):
        # 标题
        title_label = tk.Label(self.root, text="2048 - 10x10", font=("Arial", 20, "bold"), fg="#776e65")
        title_label.pack(pady=5)
        
        # 分数面板
        score_frame = tk.Frame(self.root)
        score_frame.pack(pady=5)
        
        self.score_label = tk.Label(score_frame, text="分数: 0", font=("Arial", 16, "bold"), fg="#776e65")
        self.score_label.pack(side=tk.LEFT, padx=20)
        
        # 最高值显示
        self.highest_label = tk.Label(score_frame, text="最高值: 0", font=("Arial", 16, "bold"), fg="#776e65")
        self.highest_label.pack(side=tk.LEFT, padx=20)
        
        # 游戏说明
        instructions = tk.Label(self.root, text="使用方向键或WASD移动方块，相同数字碰撞会合并", 
                                font=("Arial", 11), fg="#776e65")
        instructions.pack(pady=3)
        
        instructions2 = tk.Label(self.root, text="游戏持续直到无法移动，无胜利条件", 
                                font=("Arial", 11, "bold"), fg="#ff6600")
        instructions2.pack(pady=2)
        
        # 游戏网格框架
        self.grid_frame = tk.Frame(self.root, bg="#bbada0", padx=5, pady=5)
        self.grid_frame.pack(pady=10)
        
        # 创建单元格 - 10x10网格，增大方块尺寸
        self.cell_labels = []
        for i in range(self.grid_size):
            row_labels = []
            for j in range(self.grid_size):
                # 增大单元格尺寸（width和height乘以1.2倍）
                cell = tk.Label(self.grid_frame, text="", font=("Arial", 11, "bold"),  # 字体也相应增大
                               width=5, height=3, relief="raised", borderwidth=2)  # width=5, height=3（原为4,2）
                cell.grid(row=i, column=j, padx=3, pady=3)  # 增加内边距
                row_labels.append(cell)
            self.cell_labels.append(row_labels)
        
        # 按钮框架
        button_frame = tk.Frame(self.root)
        button_frame.pack(pady=15)
        
        restart_button = tk.Button(button_frame, text="重新开始", font=("Arial", 12), 
                                  command=self.restart_game, bg="#8f7a66", fg="white",
                                  width=12, height=1)
        restart_button.pack(side=tk.LEFT, padx=10)
        
        exit_button = tk.Button(button_frame, text="退出游戏", font=("Arial", 12), 
                               command=self.root.quit, bg="#8f7a66", fg="white",
                               width=12, height=1)
        exit_button.pack(side=tk.LEFT, padx=10)
        
        # 添加提示
        hint_label = tk.Label(self.root, text="提示：10x10网格提供适中的挑战，游戏持续到无法移动！", 
                              font=("Arial", 10), fg="#776e65")
        hint_label.pack(pady=5)
    
    def start_game(self):
        """初始化游戏，添加初始方块"""
        self.cells = np.zeros((self.grid_size, self.grid_size), dtype=int)
        self.score = 0
        self.game_over = False
        
        # 添加3个初始方块（10x10网格需要更多初始方块）
        for _ in range(3):
            self.add_random_cell()
        
        self.update_ui()
    
    def add_random_cell(self):
        """在空白位置随机添加一个数字方块（90%概率为2，10%概率为4）"""
        empty_cells = [(i, j) for i in range(self.grid_size) for j in range(self.grid_size) if self.cells[i][j] == 0]
        
        if empty_cells:
            i, j = random.choice(empty_cells)
            self.cells[i][j] = 2 if random.random() < 0.9 else 4
            return True
        return False
    
    def update_ui(self):
        """更新游戏界面"""
        # 更新分数
        self.score_label.config(text=f"分数: {self.score}")
        
        # 更新最高值
        max_value = np.max(self.cells)
        self.highest_label.config(text=f"最高值: {max_value}")
        
        # 更新单元格
        for i in range(self.grid_size):
            for j in range(self.grid_size):
                value = self.cells[i][j]
                cell_label = self.cell_labels[i][j]
                
                if value == 0:
                    cell_label.config(text="", bg=self.colors[0], fg="black")
                else:
                    # 根据数字大小调整字体，与增大的方块比例协调
                    font_size = 11  # 基础字体增大
                    if value >= 1000000:
                        font_size = 8
                    elif value >= 100000:
                        font_size = 9
                    elif value >= 10000:
                        font_size = 10
                    elif value >= 1000:
                        font_size = 11
                    
                    cell_label.config(text=str(value), 
                                     bg=self.colors.get(value, "#3c3a32"),
                                     fg=self.font_colors.get(value, "#f9f6f2"),
                                     font=("Arial", font_size, "bold"))
    
    def move(self, direction):
        """根据方向移动方块"""
        if self.game_over:
            return
        
        # 记录移动前的状态
        old_cells = self.cells.copy()
        
        # 根据方向移动和合并
        if direction == "left":
            self.move_left()
        elif direction == "right":
            self.move_right()
        elif direction == "up":
            self.move_up()
        elif direction == "down":
            self.move_down()
        
        # 检查是否有移动发生
        if not np.array_equal(old_cells, self.cells):
            self.add_random_cell()
            self.update_ui()
            
            # 检查游戏是否结束
            if self.check_game_over():
                self.game_over = True
                max_value = np.max(self.cells)
                messagebox.showinfo("游戏结束", f"游戏结束！最终得分: {self.score}\n最高数字: {max_value}")
    
    def move_left(self):
        """向左移动方块"""
        for i in range(self.grid_size):
            # 移除零值
            row = [x for x in self.cells[i] if x != 0]
            
            # 合并相同值
            j = 0
            while j < len(row) - 1:
                if row[j] == row[j + 1]:
                    row[j] *= 2
                    row.pop(j + 1)
                    self.score += row[j]
                j += 1
            
            # 填充剩余位置为零
            row.extend([0] * (self.grid_size - len(row)))
            
            # 更新行
            self.cells[i] = row
    
    def move_right(self):
        """向右移动方块"""
        for i in range(self.grid_size):
            # 移除零值并反转
            row = [x for x in self.cells[i] if x != 0]
            row.reverse()
            
            # 合并相同值
            j = 0
            while j < len(row) - 1:
                if row[j] == row[j + 1]:
                    row[j] *= 2
                    row.pop(j + 1)
                    self.score += row[j]
                j += 1
            
            # 填充剩余位置为零并反转回来
            row.extend([0] * (self.grid_size - len(row)))
            row.reverse()
            
            # 更新行
            self.cells[i] = row
    
    def move_up(self):
        """向上移动方块"""
        # 转置矩阵，然后向左移动，再转置回来
        self.cells = self.cells.T
        self.move_left()
        self.cells = self.cells.T
    
    def move_down(self):
        """向下移动方块"""
        # 转置矩阵，然后向右移动，再转置回来
        self.cells = self.cells.T
        self.move_right()
        self.cells = self.cells.T
    
    def check_game_over(self):
        """检查游戏是否结束"""
        # 检查是否有空格
        if np.any(self.cells == 0):
            return False
        
        # 检查是否有可合并的相邻方块
        for i in range(self.grid_size):
            for j in range(self.grid_size):
                current = self.cells[i][j]
                # 检查右侧
                if j < self.grid_size - 1 and current == self.cells[i][j + 1]:
                    return False
                # 检查下方
                if i < self.grid_size - 1 and current == self.cells[i + 1][j]:
                    return False
        
        return True
    
    def on_key_press(self, event):
        """处理键盘事件"""
        key = event.keysym.lower()
        
        if key in ["up", "w"]:
            self.move("up")
        elif key in ["down", "s"]:
            self.move("down")
        elif key in ["left", "a"]:
            self.move("left")
        elif key in ["right", "d"]:
            self.move("right")
        elif key == "r":
            self.restart_game()
    
    def restart_game(self):
        """重新开始游戏"""
        self.start_game()
        self.update_ui()

def main():
    root = tk.Tk()
    game = Game2048(root)
    root.mainloop()

if __name__ == "__main__":
    main()