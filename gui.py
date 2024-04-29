import tkinter as tk
from tkinter import ttk, font
import subprocess


class OsSchedulerApp:
    def __init__(self, master):
        self.master = master
        self.master.title("Os-Scheduler")
        self.master.geometry("1000x800")
        self.master.configure(bg='black')  # Set the background of the main window to black

        # Define a font
        label_font = font.Font(family='Helvetica', size=14, weight='bold')

        # Text color in RGB (41, 106, 106), converted to hex for Tkinter
        text_color = "#296a6a"

        # Padding at the top to center widgets just above the middle
        top_padding = self.master.winfo_reqheight() * 0.2  # 20% from the top of the window

        # Label for the algorithm selection
        self.label_algo = tk.Label(master, text="Select the algo", fg=text_color, bg='black', font=label_font)
        self.label_algo.pack(pady=(top_padding, 10))  # Increased padding from the top

        # Combobox for selecting the algorithm
        self.algo_selection = ttk.Combobox(master, values=["RR (round robin)", "SRTN (shortest remaining time next)", "HPF (Highest priority first)"], font=label_font)
        self.algo_selection.pack(pady=(10, 10))  # Space after combobox
        self.algo_selection.bind("<<ComboboxSelected>>", self.on_algo_selected)

        # Frame to contain dynamically created widgets
        self.dynamic_frame = tk.Frame(master, bg='black')
        self.dynamic_frame.pack(pady=(10, 10))  # Space around frame

        # Button to execute the process
        self.execute_button = tk.Button(master, text="Execute", command=self.execute_c_file, font=label_font)
        self.execute_button.pack(side=tk.BOTTOM, pady=(20, 10))  # Padding around the button

        # Error message label
        self.error_label = tk.Label(master, text="", fg="red", bg='black', font=label_font)
        self.error_label.pack(side=tk.BOTTOM, pady=(10, 20))  # Padding below the error label

    def on_algo_selected(self, event):
        algo = self.algo_selection.get()
        for widget in self.dynamic_frame.winfo_children():
            widget.destroy()

        text_color = "#296a6a"
        label_font = font.Font(family='Helvetica', size=14, weight='bold')

        if algo == "RR (round robin)":
            self.label_quantum = tk.Label(self.dynamic_frame, text="Enter your quantum", fg=text_color, bg='black', font=label_font)
            self.label_quantum.pack(pady=(20, 5))  # Additional padding above quantum label
            self.quantum_entry = tk.Entry(self.dynamic_frame, font=label_font)
            self.quantum_entry.pack()

    def validate_quantum(self):
        try:
            quantum = int(self.quantum_entry.get())
            if quantum <= 0:
                raise ValueError
            return True
        except ValueError:
            self.error_label.config(text="Quantum must be a positive integer.")
            return False

    def validate_selection(self):
        if not self.algo_selection.get():
            self.error_label.config(text="Please select an algorithm.")
            return False
        return True

    def execute_c_file(self):
        self.error_label.config(text="")
        mapped = {
            "RR" : "3",
            "SRTN" : "2",
            "HPF" : "1"
        }
        if self.algo_selection.get() == "RR (round robin)":
            if not self.validate_quantum():
                return
            quantum = self.quantum_entry.get()
            args = [mapped["RR"], quantum]  # Arguments for RR
        else:
            if not self.validate_selection():
                return
            args = [mapped[self.algo_selection.get().split()[0]],'2']  # First word, e.g., "SRTN" or "HPF"

        # Call your C program here with arguments
        try:
            print(f"Executing C program with arguments: {args}")
            subprocess.Popen(["/home/ziad/Project/Ours/OS-Hero/newgen"] + args)
        except Exception as e:
            self.error_label.config(text=f"Failed to execute C file: {e}")

def main():
    root = tk.Tk()
    app = OsSchedulerApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
