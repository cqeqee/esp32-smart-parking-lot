from PIL import Image, ImageTk
import cv2
import numpy as np
import torch
import function.utils_rotate as utils_rotate
import os
import time
import function.helper as helper
import requests
import tkinter as tk
from tkinter import simpledialog, messagebox
from threading import Thread

# Load model
yolo_LP_detect = torch.hub.load('yolov5', 'custom', path='model/LP_detector_nano_61.pt', force_reload=True, source='local')
yolo_license_plate = torch.hub.load('yolov5', 'custom', path='model/LP_ocr_nano_62.pt', force_reload=True, source='local')
yolo_license_plate.conf = 0.60

# Blynk API
blynk = f''

# Camera IP URL
url = ''

# Load plate list
plate_file = 'plate_list.txt'

def load_plate_list():
    if not os.path.exists(plate_file):
        with open(plate_file, 'w') as f:
            pass
    with open(plate_file, 'r') as f:
        return set(line.strip() for line in f if line.strip())

def save_plate_list(plates):
    with open(plate_file, 'w') as f:
        f.write("\n".join(plates))

pl = load_plate_list()

class LicensePlateApp:
    def __init__(self, root):
        self.root = root
        self.root.title("License Plate Detection")
        self.root.geometry("800x600")

        self.canvas = tk.Canvas(root, width=640, height=480)
        self.canvas.pack()

        self.start_button = tk.Button(root, text="Khởi động", command=self.start_detection, font=("Helvetica", 14))
        self.start_button.pack(side='left', padx=10)

        self.exit_button = tk.Button(root, text="Thoát", command=self.exit_detection, font=("Helvetica", 14))
        self.exit_button.pack(side='left', padx=10)

        self.manage_button = tk.Button(root, text="Quản lý biển số", command=self.manage_plate_list, font=("Helvetica", 14))
        self.manage_button.pack(side='right', padx=10)

        self.running = False

    def start_detection(self):
        if not self.running:
            self.running = True
            self.detection_thread = Thread(target=self.detect_license_plate)
            self.detection_thread.daemon = True
            self.detection_thread.start()

    def exit_detection(self):
        quit()
          
    def detect_license_plate(self):
        while self.running:
            img_resp = requests.get(url, stream=True).raw
            frame = np.asarray(bytearray(img_resp.read()), dtype=np.uint8)
            frame = cv2.imdecode(frame, cv2.IMREAD_COLOR)

            plates = yolo_LP_detect(frame, size=640)
            list_plates = plates.pandas().xyxy[0].values.tolist()

            for plate in list_plates:
                flag = 0
                x = int(plate[0])
                y = int(plate[1])
                w = int(plate[2] - plate[0])
                h = int(plate[3] - plate[1])
                crop_img = frame[y:y+h, x:x+w]

                cv2.rectangle(frame, (int(plate[0]), int(plate[1])), (int(plate[2]), int(plate[3])), color=(0, 0, 225), thickness=2)

                lp = ""
                for cc in range(0, 2):
                    for ct in range(0, 2):
                        deskewed_img = utils_rotate.deskew(crop_img, cc, ct)
                        lp = helper.read_plate(yolo_license_plate, deskewed_img)
                        if lp != "unknown":
                            if lp in pl:
                                response = requests.get(blynk)
                            cv2.putText(frame, lp, (int(plate[0]), int(plate[1] - 10)), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (36, 255, 12), 2)
                            flag = 1
                            break
                    if flag == 1:
                        break        

            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            img = Image.fromarray(frame_rgb)
            imgtk = ImageTk.PhotoImage(image=img)

            self.canvas.create_image(0, 0, anchor=tk.NW, image=imgtk)
            self.canvas.image = imgtk

            time.sleep(0.02)

    def manage_plate_list(self):
        def show_list():
            plate_list = "\n".join(pl)
            messagebox.showinfo("Danh sách", plate_list)

        def add_plate():
            new_plate = simpledialog.askstring(" ", "Nhập biển số cần thêm:")
            if new_plate not in pl and new_plate  != None :
                pl.add(new_plate)
                save_plate_list(pl)
                messagebox.showinfo("Thông báo", f"Đã thêm biển số: {new_plate}")
            elif new_plate in pl and new_plate  != None:
                messagebox.showinfo("Cảnh báo", f"Biển số đã tồn tại: {new_plate}")
        def remove_plate():
            plate_to_remove = simpledialog.askstring(" ", "Nhập biển số cần xóa:")
            if not plate_to_remove:  # Check if the input is empty
                return
            
            if plate_to_remove in pl:
                pl.remove(plate_to_remove)
                save_plate_list(pl)
                messagebox.showinfo("Thông báo", f"Đã xóa biển số: {plate_to_remove}")
            else:
                messagebox.showinfo("Cảnh báo", f"Biển số không tồn tại: {plate_to_remove}")

        def exit_button():
            list_window.destroy()
            
        list_window = tk.Toplevel(self.root)
        list_window.title("Quản lý biển số")
        list_window.geometry("300x200")

        tk.Button(list_window, text="Xem danh sách", command=show_list).pack(pady=5)
        tk.Button(list_window, text="Thêm biển số", command=add_plate).pack(pady=5)
        tk.Button(list_window, text="Xóa biển số", command=remove_plate).pack(pady=5)
        tk.Button(list_window, text="Thoát", command=exit_button).pack(pady=20)

if __name__ == "__main__":
    root = tk.Tk()
    app = LicensePlateApp(root)
    root.mainloop()
