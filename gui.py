import tkinter as tk
from tkinter import ttk, font
import subprocess
import io
from PIL import Image, ImageDraw, ImageFont

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
        top_padding = self.master.winfo_reqheight() * 0.5  # 20% from the top of the window

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
            "RR": "3",
            "SRTN": "2",
            "HPF": "1"
        }
        args = []
        if self.algo_selection.get() == "RR (round robin)":
            if not self.validate_quantum():
                return
            quantum = self.quantum_entry.get()
            args = [mapped["RR"], quantum]  # Arguments for RR
        else:
            if not self.validate_selection():
                return
            args = [mapped[self.algo_selection.get().split()[0]], '2']  # First word, e.g., "SRTN" or "HPF"

        # Update status message in the main window
        self.error_label.config(text="Your algorithm is running...", fg="#296a6a")
        self.master.update_idletasks()  # Update the GUI to reflect the label change

        # Call your C program here with arguments
        try:
            print(f"Executing C program with arguments: {args}")
            process = subprocess.Popen(["/home/ziad/Project/Ours/OS-Hero/newgen"] + args)
            process.wait()  # Wait for the subprocess to finish
            self.error_label.config(text="Finished Execution. Generating the output file.")  # Clear the message after the subprocess completes
            self.master.after(2000, lambda: self.error_label.config(text=""))
        except KeyboardInterrupt:
            self.error_label.config(text="Finished Execution. Generating the output file.")
            self.master.after(2000, lambda: self.error_label.config(text=""))
            self.display_log_file("Log file.", "/home/ziad/Project/Ours/OS-Hero/scheduler.log")
            self.display_log_file("perf file.", "/home/ziad/Project/Ours/OS-Hero/scheduler.perf")
        except Exception as e:
            self.error_label.config(text=f"Failed to execute C file: {e}")


        
    def display_log_file(self, title, path):
        try:
            # Open and read the log file
            with open(path, "r") as file:
                log_content = file.read()

            # Create an image from the log content
            image = self.text_to_image(log_content)
            self.show_image_in_tkinter(image, title=title)
        except Exception as e:
            self.error_label.config(text=f"Error displaying log file: {e}")

    def text_to_image(self, text):
        # Load a TrueType or OpenType font
        font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
        font_size = 14
        font = ImageFont.truetype(font_path, font_size)

        # Create image with white background
        width, height = self.calculate_text_size(text, font)
        image = Image.new('RGB', (width, height), color=(255, 255, 255))
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), text, font=font, fill=(0, 0, 0))

        return image

    def calculate_text_size(self, text, font):
        lines = text.split('\n')
        max_width = 0
        total_height = 0

        # Calculate maximum width and total height by examining each line
        for line in lines:
            bbox = font.getbbox(line)
            line_width = bbox[2] - bbox[0]  # width from x1 - x0
            line_height = bbox[3] - bbox[1]  # height from y1 - y0

            max_width = max(max_width, line_width)
            total_height += line_height + 10  # add 10 pixels as line spacing

        # Subtract the last added spacing for the correct total height
        total_height -= 10

        return max_width, total_height

    def show_image_in_tkinter(self, image, title):
        # Convert PIL Image to PhotoImage
        bio = io.BytesIO()
        image.save(bio, format="PNG")
        img = tk.PhotoImage(data=bio.getvalue())

        # Create a new top-level window
        new_window = tk.Toplevel(self.master)
        new_window.title(title)

        # Create a label in the new window and set the image
        label = tk.Label(new_window, image=img)
        label.image = img  # Keep a reference!
        label.pack()

        # Optional: you can also resize the window based on the image size
        new_window.geometry(f"{image.width}x{image.height}")



def main():
    root = tk.Tk()
    app = OsSchedulerApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
