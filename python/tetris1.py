import tkinter as tk
import random
import time

class Tetris:
    def __init__(self, root):
        self.root = root
        self.root.title("俄罗斯方块")
        
        # 游戏区域大小
        self.board_width = 10
        self.board_height = 20
        self.cell_size = 30
        
        # 初始化游戏状态
        self.board = [[0 for _ in range(self.board_width)] for _ in range(self.board_height)]
        self.score = 0
        self.level = 1
        self.game_over = False
        self.paused = False
        
        # 方块形状定义
        self.shapes = [
            [[1, 1, 1, 1]],  # I
            [[1, 1], [1, 1]],  # O
            [[1, 1, 1], [0, 1, 0]],  # T
            [[1, 1, 1], [1, 0, 0]],  # J
            [[1, 1, 1], [0, 0, 1]],  # L
            [[0, 1, 1], [1, 1, 0]],  # S
            [[1, 1, 0], [0, 1, 1]]   # Z
        ]
        
        # 方块颜色
        self.colors = ['cyan', 'yellow', 'purple', 'blue', 'orange', 'green', 'red']
        
        # 当前方块
        self.current_shape = None
        self.current_color = None
        self.current_x = 0
        self.current_y = 0
        
        # 下一个方块
        self.next_shape = None
        self.next_color = None
        
        # 游戏速度（毫秒）
        self.speed = 500
        
        # 创建UI
        self.setup_ui()
        
        # 初始化游戏
        self.new_game()
        
        # 绑定键盘事件
        self.root.bind('<Key>', self.key_pressed)
        
        # 开始游戏循环
        self.game_loop()
    
    def setup_ui(self):
        # 主框架
        main_frame = tk.Frame(self.root, bg='black')
        main_frame.pack(padx=10, pady=10)
        
        # 游戏区域画布
        self.canvas = tk.Canvas(main_frame, 
                                width=self.board_width * self.cell_size,
                                height=self.board_height * self.cell_size,
                                bg='black', highlightthickness=1,
                                highlightbackground='white')
        self.canvas.grid(row=0, column=0, rowspan=4, padx=(0, 20))
        
        # 信息面板
        info_frame = tk.Frame(main_frame, bg='black')
        info_frame.grid(row=0, column=1, sticky='n')
        
        # 下一个方块预览
        tk.Label(info_frame, text="下一个方块:", font=('Arial', 12), 
                bg='black', fg='white').pack(anchor='w')
        self.next_canvas = tk.Canvas(info_frame, width=4 * self.cell_size, 
                                     height=4 * self.cell_size, bg='black')
        self.next_canvas.pack(pady=(0, 20))
        
        # 分数显示
        self.score_label = tk.Label(info_frame, text=f"分数: {self.score}", 
                                   font=('Arial', 14), bg='black', fg='white')
        self.score_label.pack(anchor='w', pady=(0, 10))
        
        # 等级显示
        self.level_label = tk.Label(info_frame, text=f"等级: {self.level}", 
                                   font=('Arial', 14), bg='black', fg='white')
        self.level_label.pack(anchor='w', pady=(0, 10))
        
        # 游戏状态显示
        self.status_label = tk.Label(info_frame, text="游戏进行中", 
                                    font=('Arial', 12), bg='black', fg='green')
        self.status_label.pack(anchor='w', pady=(0, 20))
        
        # 控制按钮
        btn_frame = tk.Frame(info_frame, bg='black')
        btn_frame.pack()
        
        tk.Button(btn_frame, text="新游戏", width=10, 
                 command=self.new_game).grid(row=0, column=0, padx=5, pady=5)
        
        tk.Button(btn_frame, text="暂停/继续", width=10, 
                 command=self.toggle_pause).grid(row=0, column=1, padx=5, pady=5)
        
        # 控制说明
        controls = tk.Label(info_frame, 
                           text="控制说明:\n← → : 左右移动\n↑ : 旋转\n↓ : 加速下落\n空格 : 直接落到底",
                           font=('Arial', 10), bg='black', fg='lightgray',
                           justify='left')
        controls.pack(anchor='w', pady=(20, 0))
    
    def new_game(self):
        """开始新游戏"""
        self.board = [[0 for _ in range(self.board_width)] for _ in range(self.board_height)]
        self.score = 0
        self.level = 1
        self.speed = 500
        self.game_over = False
        self.paused = False
        
        # 生成第一个方块
        self.spawn_shape()
        self.spawn_shape()  # 生成下一个方块
        
        # 更新UI
        self.update_score()
        self.status_label.config(text="游戏进行中", fg='green')
        
        # 重绘画布
        self.draw_board()
    
    def spawn_shape(self):
        """生成新的方块"""
        if self.next_shape is None:
            # 第一次生成两个方块
            shape_idx = random.randint(0, len(self.shapes) - 1)
            self.current_shape = self.shapes[shape_idx]
            self.current_color = self.colors[shape_idx]
            self.current_x = self.board_width // 2 - len(self.current_shape[0]) // 2
            self.current_y = 0
            
            # 生成下一个方块
            shape_idx = random.randint(0, len(self.shapes) - 1)
            self.next_shape = self.shapes[shape_idx]
            self.next_color = self.colors[shape_idx]
        else:
            # 使用预先生成的下一个方块
            self.current_shape = self.next_shape
            self.current_color = self.next_color
            self.current_x = self.board_width // 2 - len(self.current_shape[0]) // 2
            self.current_y = 0
            
            # 生成新的下一个方块
            shape_idx = random.randint(0, len(self.shapes) - 1)
            self.next_shape = self.shapes[shape_idx]
            self.next_color = self.colors[shape_idx]
        
        # 绘制下一个方块预览
        self.draw_next_shape()
        
        # 检查游戏是否结束
        if self.check_collision(self.current_shape, self.current_x, self.current_y):
            self.game_over = True
            self.status_label.config(text="游戏结束!", fg='red')
    
    def draw_next_shape(self):
        """绘制下一个方块预览"""
        self.next_canvas.delete("all")
        
        shape = self.next_shape
        color = self.next_color
        
        # 计算居中显示
        offset_x = (4 - len(shape[0])) * self.cell_size // 2
        offset_y = (4 - len(shape)) * self.cell_size // 2
        
        for y, row in enumerate(shape):
            for x, cell in enumerate(row):
                if cell:
                    x1 = offset_x + x * self.cell_size
                    y1 = offset_y + y * self.cell_size
                    x2 = x1 + self.cell_size
                    y2 = y1 + self.cell_size
                    
                    self.next_canvas.create_rectangle(
                        x1, y1, x2, y2,
                        fill=color, outline='white', width=1
                    )
    
    def draw_board(self):
        """绘制整个游戏板"""
        self.canvas.delete("all")
        
        # 绘制已固定的方块
        for y in range(self.board_height):
            for x in range(self.board_width):
                if self.board[y][x]:
                    color_idx = self.board[y][x] - 1
                    color = self.colors[color_idx] if color_idx < len(self.colors) else 'white'
                    
                    x1 = x * self.cell_size
                    y1 = y * self.cell_size
                    x2 = x1 + self.cell_size
                    y2 = y1 + self.cell_size
                    
                    self.canvas.create_rectangle(
                        x1, y1, x2, y2,
                        fill=color, outline='white', width=1
                    )
        
        # 绘制当前下落的方块
        if not self.game_over and not self.paused:
            for y, row in enumerate(self.current_shape):
                for x, cell in enumerate(row):
                    if cell:
                        board_x = self.current_x + x
                        board_y = self.current_y + y
                        
                        # 只绘制在游戏区域内的部分
                        if 0 <= board_y < self.board_height:
                            x1 = board_x * self.cell_size
                            y1 = board_y * self.cell_size
                            x2 = x1 + self.cell_size
                            y2 = y1 + self.cell_size
                            
                            self.canvas.create_rectangle(
                                x1, y1, x2, y2,
                                fill=self.current_color, outline='white', width=1
                            )
        
        # 绘制网格线
        for i in range(self.board_width + 1):
            self.canvas.create_line(
                i * self.cell_size, 0,
                i * self.cell_size, self.board_height * self.cell_size,
                fill='gray', width=0.5
            )
        
        for i in range(self.board_height + 1):
            self.canvas.create_line(
                0, i * self.cell_size,
                self.board_width * self.cell_size, i * self.cell_size,
                fill='gray', width=0.5
            )
    
    def check_collision(self, shape, x, y):
        """检查方块是否与边界或其他方块碰撞"""
        for shape_y, row in enumerate(shape):
            for shape_x, cell in enumerate(row):
                if cell:
                    # 检查边界
                    board_x = x + shape_x
                    board_y = y + shape_y
                    
                    if (board_x < 0 or board_x >= self.board_width or 
                        board_y >= self.board_height):
                        return True
                    
                    # 检查与已固定方块的碰撞
                    if board_y >= 0 and self.board[board_y][board_x]:
                        return True
        return False
    
    def rotate_shape(self, shape):
        """旋转方块"""
        # 转置矩阵并反转每一行来实现90度旋转
        return [[shape[y][x] for y in range(len(shape)-1, -1, -1)] 
                for x in range(len(shape[0]))]
    
    def merge_shape(self):
        """将当前方块合并到游戏板上"""
        for y, row in enumerate(self.current_shape):
            for x, cell in enumerate(row):
                if cell:
                    board_x = self.current_x + x
                    board_y = self.current_y + y
                    
                    if 0 <= board_y < self.board_height and 0 <= board_x < self.board_width:
                        # 将颜色索引存入board（1-based）
                        color_idx = self.colors.index(self.current_color) + 1
                        self.board[board_y][board_x] = color_idx
    
    def clear_lines(self):
        """清除已填满的行并计分"""
        lines_cleared = 0
        y = self.board_height - 1
        
        while y >= 0:
            if all(self.board[y]):
                # 移除这一行
                del self.board[y]
                # 在顶部添加新的空行
                self.board.insert(0, [0 for _ in range(self.board_width)])
                lines_cleared += 1
            else:
                y -= 1
        
        # 更新分数
        if lines_cleared > 0:
            self.score += (lines_cleared ** 2) * 100
            self.level = self.score // 1000 + 1
            self.speed = max(50, 500 - (self.level - 1) * 50)  # 随等级加快速度
            self.update_score()
    
    def update_score(self):
        """更新分数和等级显示"""
        self.score_label.config(text=f"分数: {self.score}")
        self.level_label.config(text=f"等级: {self.level}")
    
    def move(self, dx, dy):
        """移动方块"""
        if self.game_over or self.paused:
            return
        
        new_x = self.current_x + dx
        new_y = self.current_y + dy
        
        if not self.check_collision(self.current_shape, new_x, new_y):
            self.current_x = new_x
            self.current_y = new_y
            self.draw_board()
            return True
        elif dy > 0:  # 向下移动失败，说明方块已经到底
            self.merge_shape()
            self.clear_lines()
            self.spawn_shape()
            self.draw_board()
        return False
    
    def rotate(self):
        """旋转方块"""
        if self.game_over or self.paused:
            return
        
        rotated = self.rotate_shape(self.current_shape)
        
        # 检查旋转后是否碰撞
        if not self.check_collision(rotated, self.current_x, self.current_y):
            self.current_shape = rotated
            self.draw_board()
            return True
        
        # 如果旋转后碰撞，尝试左右移动一格再旋转
        for dx in [-1, 1, -2, 2]:
            if not self.check_collision(rotated, self.current_x + dx, self.current_y):
                self.current_x += dx
                self.current_shape = rotated
                self.draw_board()
                return True
        
        return False
    
    def drop(self):
        """快速落到底"""
        if self.game_over or self.paused:
            return
        
        while self.move(0, 1):
            pass
    
    def game_loop(self):
        """游戏主循环"""
        if not self.game_over and not self.paused:
            self.move(0, 1)
        
        self.root.after(self.speed, self.game_loop)
    
    def key_pressed(self, event):
        """处理键盘事件"""
        if event.keysym == 'Left':
            self.move(-1, 0)
        elif event.keysym == 'Right':
            self.move(1, 0)
        elif event.keysym == 'Down':
            self.move(0, 1)
        elif event.keysym == 'Up':
            self.rotate()
        elif event.keysym == 'space':
            self.drop()
    
    def toggle_pause(self):
        """暂停/继续游戏"""
        self.paused = not self.paused
        if self.paused:
            self.status_label.config(text="游戏暂停", fg='yellow')
        else:
            self.status_label.config(text="游戏进行中", fg='green')
        self.draw_board()

def main():
    root = tk.Tk()
    game = Tetris(root)
    root.mainloop()

if __name__ == "__main__":
    main()