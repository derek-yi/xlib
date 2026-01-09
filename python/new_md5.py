import tkinter as tk
from tkinter import filedialog, ttk, messagebox
import hashlib
import os
import threading
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime

class MD5CalculatorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("高级文件MD5计算器")
        self.root.geometry("900x600")
        self.root.minsize(800, 500)
        
        # 设置中文字体
        self.style = ttk.Style()
        self.style.configure("TButton", font=("SimHei", 10))
        self.style.configure("TLabel", font=("SimHei", 10))
        self.style.configure("Treeview", font=("SimHei", 10))
        
        # 创建主框架
        self.main_frame = ttk.Frame(root, padding="20")
        self.main_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建顶部按钮区域
        self.button_frame = ttk.Frame(self.main_frame)
        self.button_frame.pack(fill=tk.X, pady=(0, 10))
        
        # 选择文件按钮
        self.select_files_btn = ttk.Button(
            self.button_frame, 
            text="选择文件", 
            command=self.select_files,
            width=15
        )
        self.select_files_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        # 选择文件夹按钮
        self.select_folder_btn = ttk.Button(
            self.button_frame, 
            text="选择文件夹", 
            command=self.select_folder,
            width=15
        )
        self.select_folder_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        # 清除结果按钮
        self.clear_btn = ttk.Button(
            self.button_frame, 
            text="清除结果", 
            command=self.clear_results,
            width=15
        )
        self.clear_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        # 验证MD5按钮
        self.verify_btn = ttk.Button(
            self.button_frame, 
            text="验证MD5", 
            command=self.verify_md5,
            width=15
        )
        self.verify_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        # 创建进度条
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            self.button_frame, 
            variable=self.progress_var, 
            length=200
        )
        self.progress_bar.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(10, 0))
        
        # 创建结果显示区域
        self.results_frame = ttk.Frame(self.main_frame)
        self.results_frame.pack(fill=tk.BOTH, expand=True)
        
        # 创建表格视图
        columns = ("filename", "size", "md5", "status", "timestamp")
        self.result_tree = ttk.Treeview(
            self.results_frame, 
            columns=columns, 
            show="headings"
        )
        
        # 设置列标题
        self.result_tree.heading("filename", text="文件名")
        self.result_tree.heading("size", text="大小")
        self.result_tree.heading("md5", text="MD5哈希值")
        self.result_tree.heading("status", text="状态")
        self.result_tree.heading("timestamp", text="计算时间")
        
        # 设置列宽
        self.result_tree.column("filename", width=250)
        self.result_tree.column("size", width=80, anchor=tk.E)
        self.result_tree.column("md5", width=320)
        self.result_tree.column("status", width=80)
        self.result_tree.column("timestamp", width=120)
        
        # 添加滚动条
        self.scrollbar_y = ttk.Scrollbar(
            self.results_frame, 
            orient=tk.VERTICAL, 
            command=self.result_tree.yview
        )
        self.scrollbar_x = ttk.Scrollbar(
            self.results_frame, 
            orient=tk.HORIZONTAL, 
            command=self.result_tree.xview
        )
        self.result_tree.configure(yscrollcommand=self.scrollbar_y.set)
        self.result_tree.configure(xscrollcommand=self.scrollbar_x.set)
        
        # 放置表格和滚动条
        self.scrollbar_y.pack(side=tk.RIGHT, fill=tk.Y)
        self.scrollbar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.result_tree.pack(fill=tk.BOTH, expand=True)
        
        # 存储已计算的文件信息
        self.file_info = {}
        
        # 创建线程池
        self.executor = ThreadPoolExecutor(max_workers=4)
        self.pending_tasks = []
        
    def select_files(self):
        """选择多个文件并计算MD5"""
        file_paths = filedialog.askopenfilenames(
            title="选择文件",
            filetypes=[("所有文件", "*.*")]
        )
        if file_paths:
            self.calculate_md5_for_files(file_paths)
    
    def select_folder(self):
        """选择文件夹并计算其中所有文件的MD5"""
        folder_path = filedialog.askdirectory(title="选择文件夹")
        if folder_path:
            # 获取文件夹中所有文件
            file_paths = []
            for root, _, files in os.walk(folder_path):
                for file in files:
                    file_paths.append(os.path.join(root, file))
            
            if file_paths:
                self.calculate_md5_for_files(file_paths)
    
    def calculate_md5_for_files(self, file_paths):
        """计算一组文件的MD5值"""
        total_files = len(file_paths)
        processed_files = 0
        
        # 清空之前的任务
        self.cancel_pending_tasks()
        self.pending_tasks = []
        
        # 更新进度条
        self.progress_var.set(0)
        
        for file_path in file_paths:
            # 提交任务到线程池
            future = self.executor.submit(self.calculate_md5_async, file_path)
            future.add_done_callback(
                lambda f, p=file_path: self.update_result_after_calculation(f, p, total_files)
            )
            self.pending_tasks.append(future)
    
    def calculate_md5_async(self, file_path):
        """在后台线程中计算MD5"""
        try:
            hash_md5 = hashlib.md5()
            file_size = os.path.getsize(file_path)
            
            with open(file_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hash_md5.update(chunk)
            
            md5_hash = hash_md5.hexdigest()
            
            # 检查是否为特定文件并保存MD5
            save_result = self.save_special_md5(file_path, md5_hash)
            
            return {
                "success": True,
                "md5": md5_hash,
                "size": self.format_size(file_size),
                "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                "save_result": save_result
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    def save_special_md5(self, file_path, md5_hash):
        """保存特定文件的MD5到同目录下的.md5文件"""
        filename = os.path.basename(file_path)
        
        if filename == "ds800_app.bin" or filename == "ds800_gold.bin":
            dir_path = os.path.dirname(file_path)
            if filename == "ds800_app.bin":
                md5_file_path = os.path.join(dir_path, "ds800_app.md5")
            else:
                md5_file_path = os.path.join(dir_path, "ds800_gold.md5")
            
            try:
                with open(md5_file_path, 'w') as f:
                    f.write(f"{md5_hash} *{filename}")
                return True
            except Exception as e:
                print(f"保存MD5文件失败: {e}")
                return False
        
        return None
    
    def update_result_after_calculation(self, future, file_path, total_files):
        """计算完成后更新结果显示"""
        result = future.result()
        filename = os.path.basename(file_path)
        
        if result["success"]:
            # 更新文件信息字典
            self.file_info[file_path] = result["md5"]
            
            # 确定状态文本
            status = "完成"
            if result["save_result"] is not None:
                status = "已保存" if result["save_result"] else "保存失败"
            
            # 更新表格
            self.result_tree.insert(
                "", 
                tk.END, 
                values=(
                    filename, 
                    result["size"], 
                    result["md5"], 
                    status, 
                    result["timestamp"]
                )
            )
        else:
            # 显示错误信息
            self.result_tree.insert(
                "", 
                tk.END, 
                values=(filename, "", f"错误: {result['error']}", "错误", "")
            )
        
        # 更新进度条
        processed_files = len(self.result_tree.get_children())
        self.progress_var.set(100 * processed_files / total_files)
        
        # 从待处理任务列表中移除
        self.pending_tasks.remove(future)
    
    def cancel_pending_tasks(self):
        """取消所有待处理的任务"""
        for future in self.pending_tasks:
            if not future.done():
                future.cancel()
        self.pending_tasks = []
    
    def clear_results(self):
        """清除所有结果"""
        self.cancel_pending_tasks()
        for item in self.result_tree.get_children():
            self.result_tree.delete(item)
        self.file_info.clear()
        self.progress_var.set(0)
    
    def verify_md5(self):
        """验证MD5值"""
        selected_items = self.result_tree.selection()
        if not selected_items:
            messagebox.showinfo("提示", "请先选择要验证的文件")
            return
        
        for item in selected_items:
            values = self.result_tree.item(item, "values")
            filename, _, stored_md5, status, _ = values
            
            # 跳过已验证或错误的条目
            if status != "完成" and status != "已保存":
                continue
            
            # 查找文件路径
            file_path = None
            for path, md5 in self.file_info.items():
                if md5 == stored_md5 and os.path.basename(path) == filename:
                    file_path = path
                    break
            
            if file_path:
                # 重新计算MD5
                result = self.calculate_md5_async(file_path)
                if result["success"]:
                    new_md5 = result["md5"]
                    if new_md5 == stored_md5:
                        self.result_tree.item(item, values=(
                            filename, 
                            result["size"], 
                            stored_md5, 
                            "验证通过", 
                            datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        ))
                    else:
                        self.result_tree.item(item, values=(
                            filename, 
                            result["size"], 
                            stored_md5, 
                            "验证失败", 
                            datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        ))
    
    def format_size(self, size_bytes):
        """格式化文件大小显示"""
        units = ["B", "KB", "MB", "GB", "TB"]
        unit_index = 0
        
        while size_bytes >= 1024 and unit_index < len(units) - 1:
            size_bytes /= 1024
            unit_index += 1
        
        return f"{size_bytes:.2f} {units[unit_index]}"

if __name__ == "__main__":
    root = tk.Tk()
    app = MD5CalculatorApp(root)
    root.mainloop()    