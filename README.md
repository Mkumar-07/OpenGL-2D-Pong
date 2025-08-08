# Pong (OpenGL + GLFW)

A simple 2D Pong clone built in **C++** using **OpenGL**, **GLFW**, and **GLAD**.

---

## 📦 Dependencies

If you don’t have **GLFW**/**GLAD**, install them via your package manager or build them from source.

### On Linux (Ubuntu/Debian)
```bash
sudo apt install libglfw3-dev libgl1-mesa-dev
```

### On macOS
```bash
brew install glfw
```

### On Windows
Use [vcpkg](https://vcpkg.io/) or manually include prebuilt GLFW and GLAD files.

---

## 🛠️ Build

### Using g++
```bash
g++ main.cpp -o pong -lglfw -ldl -lGL
```

### On macOS
```bash
g++ main.cpp -o pong -lglfw -framework OpenGL
```

---

## ▶️ Run
```bash
./pong
```

---

## 🎮 Controls

| Key        | Action                     |
|------------|----------------------------|
| **W / S**  | Move left paddle up/down    |
| **↑ / ↓**  | Move right paddle up/down   |
| **Esc**    | Quit the game               |

---

## 📂 Project Structure
```
.
├── main.cpp         # Game source code
├── main.vs          # Vertex shader
├── main.fs          # Fragment shader
├── README.md        # Project documentation
└── docs/            # (Optional) screenshots, extra docs
```

---

## 🖥️ How It Works
- Uses **GLFW** for window creation & input
- Uses **GLAD** to load OpenGL function pointers
- Orthographic projection for 2D gameplay
- Instanced rendering for paddles
- Circle model generated procedurally for the ball
- Simple axis-aligned collision detection
