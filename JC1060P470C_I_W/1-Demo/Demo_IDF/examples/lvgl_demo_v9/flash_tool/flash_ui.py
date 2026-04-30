import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import threading
import subprocess
import sys
import os
import urllib.request
import tempfile

# ─── Auto install missing packages ───────────────────────────────────────────
def install_package(package):
    subprocess.check_call([sys.executable, "-m", "pip", "install", package, "--quiet"])

try:
    import serial.tools.list_ports
except ImportError:
    install_package("pyserial")
    import serial.tools.list_ports

try:
    import esptool
except ImportError:
    install_package("esptool")
    import esptool

# ─── SPIFFS Generator ─────────────────────────────────────────────────────────
SPIFFSGEN_URL = "https://raw.githubusercontent.com/espressif/esp-idf/master/components/spiffs/spiffsgen.py"
SPIFFSGEN_LOCAL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "spiffsgen.py")

PARTITION_SIZE    = 0x700000   # 7MB  — matches your partition table
FLASH_ADDRESS     = 0x810000   # spiffs partition offset
CHIP              = "esp32p4"
BAUD              = 921600

def get_spiffsgen(log):
    if os.path.exists(SPIFFSGEN_LOCAL):
        return SPIFFSGEN_LOCAL
    log("Downloading spiffsgen.py from Espressif...")
    try:
        urllib.request.urlretrieve(SPIFFSGEN_URL, SPIFFSGEN_LOCAL)
        log("Downloaded spiffsgen.py OK")
        return SPIFFSGEN_LOCAL
    except Exception as e:
        log(f"ERROR downloading spiffsgen.py: {e}")
        return None

def generate_spiffs(data_folder, output_bin, log):
    spiffsgen = get_spiffsgen(log)
    if spiffsgen is None:
        return False

    log(f"Packing files from: {data_folder}")
    result = subprocess.run(
        [sys.executable, spiffsgen, hex(PARTITION_SIZE), data_folder, output_bin],
        capture_output=True, text=True
    )
    if result.returncode == 0:
        size = os.path.getsize(output_bin)
        log(f"Image created: {output_bin} ({size:,} bytes)")
        return True
    else:
        log(f"ERROR: {result.stderr.strip()}")
        return False

def flash_device(port, bin_path, log):
    log(f"Connecting to {port}...")
    try:
        old_argv = sys.argv[:]
        sys.argv = [
            'esptool',
            '--chip', CHIP,
            '--port', port,
            '--baud', str(BAUD),
            'write-flash',
            hex(FLASH_ADDRESS),
            bin_path
        ]
        esptool.main()
        sys.argv = old_argv
        return True
    except SystemExit as e:
        sys.argv = old_argv
        if str(e) == '0':
            return True
        log(f"Flash error code: {e}")
        return False
    except Exception as e:
        sys.argv = old_argv
        log(f"ERROR: {e}")
        return False


# ─── GUI App ──────────────────────────────────────────────────────────────────
class FlashApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("WOHNUX-Innovating Something New")
        self.geometry("620x580")
        self.resizable(False, False)
        self.configure(bg="#1a1a2e")

        self.devices_csv = tk.StringVar()
        self.scenes_csv  = tk.StringVar()
        self.config_csv  = tk.StringVar()
        self.com_port    = tk.StringVar()

        self._build_ui()
        self._refresh_ports()

    # ── UI Builder ────────────────────────────────────────────────────────────
    def _build_ui(self):
        # Header
        header = tk.Frame(self, bg="#16213e", pady=18)
        header.pack(fill="x")

        tk.Label(header, text="WOHNUX FLASH TOOL",
                 font=("Courier New", 20, "bold"),
                 fg="#00d4ff", bg="#16213e").pack()
        tk.Label(header, text="Update device UI from CSV files",
                 font=("Courier New", 9),
                 fg="#4a9aba", bg="#16213e").pack()

        # Main content
        content = tk.Frame(self, bg="#1a1a2e", padx=30, pady=20)
        content.pack(fill="both", expand=True)

        # ── File selection ────────────────────────────────────────────────────
        self._section(content, "STEP 1 — SELECT CSV FILES")

        self._file_row(content, "devices.csv", self.devices_csv,
                       lambda: self._pick_file(self.devices_csv, "devices.csv"))
        self._file_row(content, "scenes.csv",  self.scenes_csv,
                       lambda: self._pick_file(self.scenes_csv,  "scenes.csv"))
        self._file_row(content, "config.csv",  self.config_csv,
                       lambda: self._pick_file(self.config_csv,  "config.csv"))

        tk.Frame(content, bg="#1a1a2e", height=15).pack()

        # ── Port selection ────────────────────────────────────────────────────
        self._section(content, "STEP 2 — SELECT COM PORT")

        port_frame = tk.Frame(content, bg="#1a1a2e")
        port_frame.pack(fill="x", pady=4)

        self.port_combo = ttk.Combobox(port_frame, textvariable=self.com_port,
                                        width=18, font=("Courier New", 11),
                                        state="readonly")
        self.port_combo.pack(side="left")

        tk.Button(port_frame, text="⟳ Refresh",
                  font=("Courier New", 9), bg="#0f3460", fg="#00d4ff",
                  relief="flat", padx=10, cursor="hand2",
                  command=self._refresh_ports).pack(side="left", padx=10)

        tk.Frame(content, bg="#1a1a2e", height=10).pack()

        # ── Flash button ──────────────────────────────────────────────────────
        self._section(content, "STEP 3 — FLASH TO DEVICE")

        self.flash_btn = tk.Button(content,
                                    text="▶  FLASH NOW",
                                    font=("Courier New", 13, "bold"),
                                    bg="#00d4ff", fg="#1a1a2e",
                                    relief="flat", pady=12,
                                    cursor="hand2",
                                    command=self._start_flash)
        self.flash_btn.pack(fill="x", pady=6)

        # ── Progress bar ──────────────────────────────────────────────────────
        self.progress = ttk.Progressbar(content, mode="indeterminate", length=400)
        self.progress.pack(fill="x", pady=4)

        # ── Log area ──────────────────────────────────────────────────────────
        self._section(content, "LOG")

        log_frame = tk.Frame(content, bg="#0d1117", pady=2, padx=2)
        log_frame.pack(fill="both", expand=True)

        self.log_box = tk.Text(log_frame,
                                height=8,
                                font=("Courier New", 9),
                                bg="#0d1117", fg="#58d68d",
                                relief="flat", padx=6, pady=4,
                                state="disabled",
                                wrap="word")
        self.log_box.pack(fill="both", expand=True)

        scroll = ttk.Scrollbar(log_frame, command=self.log_box.yview)
        self.log_box.configure(yscrollcommand=scroll.set)

    def _section(self, parent, title):
        tk.Label(parent, text=title,
                 font=("Courier New", 8, "bold"),
                 fg="#4a9aba", bg="#1a1a2e",
                 anchor="w").pack(fill="x", pady=(8, 2))
        tk.Frame(parent, bg="#0f3460", height=1).pack(fill="x")

    def _file_row(self, parent, label, var, cmd):
        row = tk.Frame(parent, bg="#1a1a2e")
        row.pack(fill="x", pady=4)

        tk.Label(row, text=label, width=14, anchor="w",
                 font=("Courier New", 10), fg="#aaaaaa",
                 bg="#1a1a2e").pack(side="left")

        entry = tk.Entry(row, textvariable=var, font=("Courier New", 9),
                         bg="#0d1117", fg="#58d68d", relief="flat",
                         insertbackground="white", width=34)
        entry.pack(side="left", padx=(0, 8))

        tk.Button(row, text="Browse",
                  font=("Courier New", 8), bg="#0f3460", fg="#00d4ff",
                  relief="flat", padx=8, cursor="hand2",
                  command=cmd).pack(side="left")

    # ── Actions ───────────────────────────────────────────────────────────────
    def _pick_file(self, var, filename):
        path = filedialog.askopenfilename(
            title=f"Select {filename}",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
        )
        if path:
            var.set(path)

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            self.com_port.set(ports[0])
        else:
            self.com_port.set("")
        self.log(f"Found ports: {ports if ports else 'none'}")

    def log(self, msg):
        self.log_box.configure(state="normal")
        self.log_box.insert("end", f"▸ {msg}\n")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")
        self.update_idletasks()

    def log_success(self, msg):
        self.log_box.configure(state="normal")
        self.log_box.insert("end", f"✓ {msg}\n", "success")
        self.log_box.tag_config("success", foreground="#2ecc71")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")
        self.update_idletasks()

    def log_error(self, msg):
        self.log_box.configure(state="normal")
        self.log_box.insert("end", f"✗ {msg}\n", "error")
        self.log_box.tag_config("error", foreground="#e74c3c")
        self.log_box.see("end")
        self.log_box.configure(state="disabled")
        self.update_idletasks()

    def _start_flash(self):
        # Validate inputs
        dev_csv = self.devices_csv.get().strip()
        scn_csv = self.scenes_csv.get().strip()
        cfg_csv = self.config_csv.get().strip()
        port    = self.com_port.get().strip()

        if not dev_csv or not os.path.exists(dev_csv):
            messagebox.showerror("Error", "Please select a valid devices.csv file")
            return
        if not scn_csv or not os.path.exists(scn_csv):
            messagebox.showerror("Error", "Please select a valid scenes.csv file")
            return
        if not cfg_csv or not os.path.exists(cfg_csv):
            messagebox.showerror("Error", "Please select a valid config.csv file")
            return
        if not port:
            messagebox.showerror("Error", "Please select a COM port")
            return

        # Disable button, start progress
        self.flash_btn.config(state="disabled", text="Working...")
        self.progress.start(10)

        # Run in background thread so UI stays responsive
        thread = threading.Thread(target=self._do_flash,
                                   args=(dev_csv, scn_csv,cfg_csv, port),
                                   daemon=True)
        thread.start()

    def _do_flash(self, dev_csv, scn_csv, cfg_csv, port):
        try:
            # ── Step 1: Copy CSVs to a temp data folder ───────────────────────
            self.log("Preparing files...")
            tmp_dir = tempfile.mkdtemp()
            import shutil
            shutil.copy(dev_csv, os.path.join(tmp_dir, "devices.csv"))
            shutil.copy(scn_csv, os.path.join(tmp_dir, "scenes.csv"))
            shutil.copy(cfg_csv, os.path.join(tmp_dir, "config.csv"))

            self.log(f"Copied CSVs to temp folder")

            # ── Step 2: Generate SPIFFS image ─────────────────────────────────
            self.log("Converting CSV files to SPIFFS image...")
            bin_path = os.path.join(tmp_dir, "spiffs.bin")
            ok = generate_spiffs(tmp_dir, bin_path, self.log)

            if not ok:
                self.log_error("Failed to generate SPIFFS image!")
                self._flash_done(False)
                return

            # ── Step 3: Flash ─────────────────────────────────────────────────
            self.log(f"Flashing to {port}... (do not unplug)")
            ok = flash_device(port, bin_path, self.log)

            if ok:
                self.log_success("DONE! Device is rebooting with new UI.")
            else:
                self.log_error("Flash failed! Check cable and COM port.")

            self._flash_done(ok)

        except Exception as e:
            self.log_error(f"Unexpected error: {e}")
            self._flash_done(False)

    def _flash_done(self, success):
        self.progress.stop()
        self.flash_btn.config(state="normal", text="▶  FLASH NOW")
        if success:
            self.flash_btn.config(bg="#2ecc71")
            self.after(3000, lambda: self.flash_btn.config(bg="#00d4ff"))


# ─── Entry point ─────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = FlashApp()
    app.mainloop()
