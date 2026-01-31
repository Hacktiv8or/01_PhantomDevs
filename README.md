# 01_PhantomDevs
# Tech Vision
### A product made by `Hacktiv8or`, `the-visible-ghost` & `ChilledWater`
> This is a product for to assist visually impared people by providing assistance in their surroundings using auditory cues.

### Key Features
- Obstacle Detection
- Danger Recognition
- SOS Signal

### Future Goals
- GPS based Navigation
- Obstacle resolution
- Emergency Alerts
- Sensor based distance measurement
- Software update/reset using mobile app

## Components Used
- ESP32-S3-N16R8 + OV3660 Camera
- Li-Ion Battery
- USB C-Type Charging module
- Custom 3.5mm Audio (aux) port
- 3.5mm Audio jack (aux) Earphone

## Working
- The camera captures the environment and sends it to a ML model
- The Object Detection model then detects objects in the image with their screen-positions and screen-sizes
- The detected objects with metadata are then used to provide assistant to the user using auditory cues.
