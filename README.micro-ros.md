# STM32 ↔ Ubuntu 24.04 qua micro-ROS Jazzy

STM32 chạy micro-ROS Client qua cổng Ethernet, dùng IP tĩnh 192.168.1.67/24.
Ubuntu chạy Agent ở UDP 8888. Firmware mặc định trỏ tới 192.168.1.100.
Node /robotnew2_stm32 gửi bộ đếm UInt32 lên /robotnew2/heartbeat mỗi giây.
Task tự thử lại khi Agent mất kết nối. Điều khiển động cơ vẫn qua PS2.

## 1. Build firmware trong STM32CubeIDE

Import project trong thư mục STM32CubeIDE. Tạo thư viện Jazzy bằng Docker
từ thư mục STM32CubeIDE (có tệp .cproject). Ví dụ trong PowerShell:

```powershell
git clone --branch jazzy --single-branch https://github.com/micro-ROS/micro_ros_stm32cubemx_utils.git
docker run --rm -v "${PWD}:/project" -e MICROROS_LIBRARY_FOLDER=micro_ros_stm32cubemx_utils/microros_static_library_ide microros/micro_ros_static_library_builder:jazzy
```

Trong cấu hình build của project:

1. Thêm thư mục include
   micro_ros_stm32cubemx_utils/microros_static_library_ide/libmicroros/include
   ở MCU GCC Compiler → Include paths.
2. Thêm đường dẫn tuyệt đối tới thư mục libmicroros ở MCU GCC Linker →
   Libraries → Library search path. Thêm microros vào Libraries.
3. Thêm ba tệp nguồn trong extra_sources của utils vào Application/User/Core
   dưới dạng linked files: microros_time.c, microros_allocators.c và
   custom_memory_manager.c. Transport UDP đã nằm trong robot_microros.c.
4. Thêm symbol ROBOT_MICROROS ở MCU GCC Compiler → Preprocessor.
   Nếu Ubuntu dùng IP khác, thêm symbol ROBOT_MICROROS_AGENT_IP với giá trị
   chuỗi C của IP đó, hoặc sửa giá trị mặc định trong Core/Src/robot_microros.c.
5. Kiểm tra thư viện và firmware cùng dùng các cờ CPU/ABI
   -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard.
   Clean, build lại và nạp firmware qua ST-LINK.

Có thể build cấu hình PS2 cũ khi chưa bật symbol ROBOT_MICROROS.

## 2. Chạy Agent trên Ubuntu

Nối Ethernet STM32 với Ubuntu hoặc cùng switch. Nếu nối trực tiếp, gán
192.168.1.100/24 cho cổng Ethernet của Ubuntu. Dùng ip -br addr để xem tên cổng.

```bash
sudo ip addr add 192.168.1.100/24 dev <ten-cong-ethernet>
sudo ip link set <ten-cong-ethernet> up
docker run --rm --network host -v /dev/shm:/dev/shm \
  microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```

Nếu đã cài Agent native, chạy ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888 -v6.

## 3. Kiểm tra kết nối

Mở terminal khác trên Ubuntu có ROS 2 Jazzy, dùng ROS_DOMAIN_ID=0:

```bash
source /opt/ros/jazzy/setup.bash
export ROS_DOMAIN_ID=0
ros2 node list
ros2 topic echo /robotnew2/heartbeat std_msgs/msg/UInt32
```

Kết nối hoạt động khi thấy node /robotnew2_stm32 và trường data tăng mỗi giây.
Có thể dừng rồi chạy lại Agent để kiểm tra STM32 tự kết nối lại.

Tham khảo [micro-ROS cho STM32CubeIDE](https://github.com/micro-ROS/micro_ros_stm32cubemx_utils/tree/jazzy#using-this-package-with-stm32cubeide).
