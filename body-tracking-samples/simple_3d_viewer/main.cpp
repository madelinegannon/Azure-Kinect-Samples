// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <array>
#include <iostream>
#include <map>
#include <vector>
#include <k4arecord/playback.h>
#include <k4a/k4a.h>
#include <k4abt.h>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

#include <BodyTrackingHelpers.h>
#include <Utilities.h>
#include <Window3dWrapper.h>

using namespace std;
using boost::asio::ip::udp;
using namespace nlohmann;

// Declare global socket variable
boost::asio::io_context io_context;
udp::socket* send_socket = nullptr;
udp::resolver::results_type* endpoints = nullptr;


/**
 * @brief Create a UDP Sender.
 *
 * @param (string)  ip_address: host address
 * @param (int)  port: send port
 */
void setup_udp(string ip_address, int port) {
    try {
        send_socket = new udp::socket(io_context, udp::endpoint(udp::v4(), 0));

        udp::resolver resolver(io_context);
        endpoints = new udp::resolver::results_type(resolver.resolve(udp::v4(), ip_address, std::to_string(port)));
        std::cout << "UDP Connection Established: " << ip_address << ":" << port << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Socket Setup Error: " << e.what() << std::endl;
        if (send_socket) {
            delete send_socket;
            send_socket = nullptr;
        }
        if (endpoints) {
            delete endpoints;
            endpoints = nullptr;
        }
    }
}

/**
 * @brief Close the UDP Connection.
 */
void shutdown_udp() {
    // Close the Socket
    if (send_socket) {
        send_socket->shutdown(boost::asio::socket_base::shutdown_both);
        send_socket->close();
        // Delete global variables
        delete send_socket;
        send_socket = nullptr;
    }
    if (endpoints) {
        // Delete global variables
        delete endpoints;
        endpoints = nullptr;
    }
}

/**
 * @brief Send a message over UDP.
 *
 * @param (string)  message_str: message to send.
 */
void send_message(const string& message_str) {
    try {
        for (auto& endpoint : *endpoints) {
            send_socket->send_to(boost::asio::buffer(message_str), endpoint);
        }
    }
    catch (std::exception& e) {
        cerr << "Error sending message: " << e.what() << endl;
    }
}



vector<string> joint_names =
{
    "K4ABT_JOINT_PELVIS",
    "K4ABT_JOINT_SPINE_NAVEL",
    "K4ABT_JOINT_SPINE_CHEST",
    "K4ABT_JOINT_NECK",
    "K4ABT_JOINT_CLAVICLE_LEFT",
    "K4ABT_JOINT_SHOULDER_LEFT",
    "K4ABT_JOINT_ELBOW_LEFT",
    "K4ABT_JOINT_WRIST_LEFT",
    "K4ABT_JOINT_HAND_LEFT",
    "K4ABT_JOINT_HANDTIP_LEFT",
    "K4ABT_JOINT_THUMB_LEFT",
    "K4ABT_JOINT_CLAVICLE_RIGHT",
    "K4ABT_JOINT_SHOULDER_RIGHT",
    "K4ABT_JOINT_ELBOW_RIGHT",
    "K4ABT_JOINT_WRIST_RIGHT",
    "K4ABT_JOINT_HAND_RIGHT",
    "K4ABT_JOINT_HANDTIP_RIGHT",
    "K4ABT_JOINT_THUMB_RIGHT",
    "K4ABT_JOINT_HIP_LEFT",
    "K4ABT_JOINT_KNEE_LEFT",
    "K4ABT_JOINT_ANKLE_LEFT",
    "K4ABT_JOINT_FOOT_LEFT",
    "K4ABT_JOINT_HIP_RIGHT",
    "K4ABT_JOINT_KNEE_RIGHT",
    "K4ABT_JOINT_ANKLE_RIGHT",
    "K4ABT_JOINT_FOOT_RIGHT",
    "K4ABT_JOINT_HEAD",
    "K4ABT_JOINT_NOSE",
    "K4ABT_JOINT_EYE_LEFT",
    "K4ABT_JOINT_EAR_LEFT",
    "K4ABT_JOINT_EYE_RIGHT",
    "K4ABT_JOINT_EAR_RIGHT",
    "K4ABT_JOINT_COUNT"
};

void PrintUsage()
{
#ifdef _WIN32
    printf("\nUSAGE: (k4abt_)simple_3d_viewer.exe SensorMode[NFOV_UNBINNED, WFOV_BINNED](optional) RuntimeMode[CPU, CUDA, DIRECTML, TENSORRT](optional) -model MODEL_PATH(optional) -id DEVICE_INDEX(optional) -ip IP_ADDRESS(optional) -port PORT(optional)\n");
#else
    printf("\nUSAGE: (k4abt_)simple_3d_viewer.exe SensorMode[NFOV_UNBINNED, WFOV_BINNED](optional) RuntimeMode[CPU, CUDA, TENSORRT](optional) -ip IP_ADDRESS(optional) -port PORT(optional)\n");
#endif
    printf("  - SensorMode: \n");
    printf("      NFOV_UNBINNED (default) - Narrow Field of View Unbinned Mode [Resolution: 640x576; FOI: 75 degree x 65 degree]\n");
    printf("      WFOV_BINNED             - Wide Field of View Binned Mode [Resolution: 512x512; FOI: 120 degree x 120 degree]\n");
    printf("  - RuntimeMode: \n");
    printf("      CPU - Use the CPU only mode. It runs on machines without a GPU but it will be much slower\n");
    printf("      CUDA - Use CUDA for processing.\n");
#ifdef _WIN32
    printf("      DIRECTML - Use the DirectML processing mode.\n");
#endif
    printf("      TENSORRT - Use the TensorRT processing mode.\n");
    printf("      OFFLINE - Play a specified file. Does not require Kinect device\n");
    printf("      DEVICE_INDEX - Index of which Kinect to open. Defaults to 0.\n");
    printf("      IP_ADDRESS - Stream Skeleton Data over UDP as JSON blob to {host:port}. Defaults to 127.0.0.1:55555.\n");
    printf("      HOST - Stream Skeleton Data over UDP as JSON blob to {host:port}. Defaults to 127.0.0.1:55555.\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe WFOV_BINNED CPU\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe CPU\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe WFOV_BINNED\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe OFFLINE MyFile.mkv\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe CPU -ip 192.168.1.100 -port 12345\n");
}

void PrintAppUsage()
{
    printf("\n");
    printf(" Basic Navigation:\n\n");
    printf(" Rotate: Rotate the camera by moving the mouse while holding mouse left button\n");
    printf(" Pan: Translate the scene by holding Ctrl key and drag the scene with mouse left button\n");
    printf(" Zoom in/out: Move closer/farther away from the scene center by scrolling the mouse scroll wheel\n");
    printf(" Select Center: Center the scene based on a detected joint by right clicking the joint with mouse\n");
    printf("\n");
    printf(" Key Shortcuts\n\n");
    printf(" ESC: quit\n");
    printf(" h: help\n");
    printf(" b: body visualization mode\n");
    printf(" k: 3d window layout\n");
    printf("\n");
}

// Global State and Key Process Function
bool s_isRunning = true;
Visualization::Layout3d s_layoutMode = Visualization::Layout3d::OnlyMainView;
bool s_visualizeJointFrame = false;


int64_t ProcessKey(void* /*context*/, int key)
{
    // https://www.glfw.org/docs/latest/group__keys.html
    switch (key)
    {
        // Quit
    case GLFW_KEY_ESCAPE:
        s_isRunning = false;
        break;
    case GLFW_KEY_K:
        s_layoutMode = (Visualization::Layout3d)(((int)s_layoutMode + 1) % (int)Visualization::Layout3d::Count);
        break;
    case GLFW_KEY_B:
        s_visualizeJointFrame = !s_visualizeJointFrame;
        break;
    case GLFW_KEY_H:
        PrintAppUsage();
        break;
    }
    return 1;
}

int64_t CloseCallback(void* /*context*/)
{
    s_isRunning = false;
    return 1;
}

struct InputSettings
{
    k4a_depth_mode_t DepthCameraMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
#ifdef _WIN32
    k4abt_tracker_processing_mode_t processingMode = K4ABT_TRACKER_PROCESSING_MODE_GPU_DIRECTML;
#else
    k4abt_tracker_processing_mode_t processingMode = K4ABT_TRACKER_PROCESSING_MODE_GPU_CUDA;
#endif
    bool Offline = false;
    std::string FileName;
    std::string ModelPath;
    std::string ip_address;
    int device_index;
    int port;
};

bool ParseInputSettingsFromArg(int argc, char** argv, InputSettings& inputSettings)
{
    for (int i = 1; i < argc; i++)
    {
        std::string inputArg(argv[i]);
        if (inputArg == std::string("NFOV_UNBINNED"))
        {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        }
        else if (inputArg == std::string("WFOV_BINNED"))
        {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        }
        else if (inputArg == std::string("CPU"))
        {
            inputSettings.processingMode = K4ABT_TRACKER_PROCESSING_MODE_CPU;
        }
        else if (inputArg == std::string("TENSORRT"))
        {
            inputSettings.processingMode = K4ABT_TRACKER_PROCESSING_MODE_GPU_TENSORRT;
        }
        else if (inputArg == std::string("CUDA"))
        {
            inputSettings.processingMode = K4ABT_TRACKER_PROCESSING_MODE_GPU_CUDA;
        }
#ifdef _WIN32
        else if (inputArg == std::string("DIRECTML"))
        {
            inputSettings.processingMode = K4ABT_TRACKER_PROCESSING_MODE_GPU_DIRECTML;
        }
#endif
        else if (inputArg == std::string("OFFLINE"))
        {
            inputSettings.Offline = true;
            if (i < argc - 1) {
                // Take the next argument after OFFLINE as file name
                inputSettings.FileName = argv[i + 1];
                i++;
            }
            else {
                return false;
            }
        }
        else if (inputArg == std::string("-model"))
        {
            if (i < argc - 1)
                inputSettings.ModelPath = argv[++i];
            else
            {
                printf("Error: model path missing\n");
                return false;
            }
        }
        else if (inputArg == std::string("-id"))
        {
            if (i < argc - 1)
                inputSettings.device_index = stoi(argv[++i]);
            else
            {
                printf("Error: device index missing\n");
                return false;
            }
        }
        else if (inputArg == std::string("-ip"))
        {
            if (i < argc - 1)
                inputSettings.ip_address = argv[++i];
            else
            {
                printf("Error: ip address missing\n");
                return false;
            }
        }
        else if (inputArg == std::string("-port"))
        {
            if (i < argc - 1)
                inputSettings.port = stoi(argv[++i]);
            else
            {
                printf("Error: ip address missing\n");
                return false;
            }
        }
        else
        {
            printf("Error: command not understood: %s\n", inputArg.c_str());
            return false;
        }
    }
    return true;
}

void VisualizeResult(k4abt_frame_t bodyFrame, Window3dWrapper& window3d, int depthWidth, int depthHeight) {

    // Obtain original capture that generates the body tracking result
    k4a_capture_t originalCapture = k4abt_frame_get_capture(bodyFrame);
    k4a_image_t depthImage = k4a_capture_get_depth_image(originalCapture);

    std::vector<Color> pointCloudColors(depthWidth * depthHeight, { 1.f, 1.f, 1.f, 1.f });

    // Read body index map and assign colors
    k4a_image_t bodyIndexMap = k4abt_frame_get_body_index_map(bodyFrame);
    const uint8_t* bodyIndexMapBuffer = k4a_image_get_buffer(bodyIndexMap);
    for (int i = 0; i < depthWidth * depthHeight; i++)
    {
        uint8_t bodyIndex = bodyIndexMapBuffer[i];
        if (bodyIndex != K4ABT_BODY_INDEX_MAP_BACKGROUND)
        {
            uint32_t bodyId = k4abt_frame_get_body_id(bodyFrame, bodyIndex);
            pointCloudColors[i] = g_bodyColors[bodyId % g_bodyColors.size()];
        }
    }
    k4a_image_release(bodyIndexMap);

    // Visualize point cloud
    window3d.UpdatePointClouds(depthImage, pointCloudColors);

    // Visualize the skeleton data
    window3d.CleanJointsAndBones();
    uint32_t numBodies = k4abt_frame_get_num_bodies(bodyFrame);
    uint64_t timestamp = k4abt_frame_get_device_timestamp_usec(bodyFrame);
    // Setup the JSON message
    json frame_result_json;
    frame_result_json["timestamp_usec"] = timestamp;
    frame_result_json["num_bodies"] = numBodies;// num_bodies;
    frame_result_json["bodies"] = json::array();
    for (uint32_t i = 0; i < numBodies; i++)
    {
        k4abt_body_t body;
        VERIFY(k4abt_frame_get_body_skeleton(bodyFrame, i, &body.skeleton), "Get skeleton from body frame failed!");
        body.id = k4abt_frame_get_body_id(bodyFrame, i);

        json body_result_json;
        body_result_json["body_id"] = body.id;

        // Assign the correct color based on the body id
        Color color = g_bodyColors[body.id % g_bodyColors.size()];
        color.a = 0.4f;
        Color lowConfidenceColor = color;
        lowConfidenceColor.a = 0.1f;

        // Visualize joints
        for (int joint = 0; joint < static_cast<int>(K4ABT_JOINT_COUNT); joint++)
        {
            if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
            {
                const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                // Add joint position to the JSON message
                body_result_json[joint_names[joint]].push_back({ jointPosition.xyz.x,
                                                                jointPosition.xyz.y,
                                                                jointPosition.xyz.z });

                window3d.AddJoint(
                    jointPosition,
                    jointOrientation,
                    body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
            }
        }
        // Add the body to the JSON message
        frame_result_json["bodies"].push_back(body_result_json);
        // Visualize bones
        for (size_t boneIdx = 0; boneIdx < g_boneList.size(); boneIdx++)
        {
            k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
            k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

            if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
            {
                bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
            }
        }
    }   
    // send the json message over udp
    if (socket && endpoints && numBodies > 0) {
        //cout << frame_result_json << endl;
        auto msg = frame_result_json.dump();
        send_message(msg);
    }
    k4a_capture_release(originalCapture);
    k4a_image_release(depthImage);

}


void PlayFile(InputSettings inputSettings)
{
    // Initialize the 3d window controller
    Window3dWrapper window3d;

    //create the tracker and playback handle
    k4a_calibration_t sensorCalibration;
    k4abt_tracker_t tracker = nullptr;
    k4a_playback_t playbackHandle = nullptr;

    const char* file = inputSettings.FileName.c_str();
    if (k4a_playback_open(file, &playbackHandle) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to open recording: %s\n", file);
        return;
    }

    if (k4a_playback_get_calibration(playbackHandle, &sensorCalibration) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to get calibration\n");
        return;
    }

    k4a_capture_t capture = nullptr;
    k4a_stream_result_t playbackResult = K4A_STREAM_RESULT_SUCCEEDED;

    k4abt_tracker_configuration_t trackerConfig = K4ABT_TRACKER_CONFIG_DEFAULT;
    trackerConfig.processing_mode = inputSettings.processingMode;
    trackerConfig.model_path = inputSettings.ModelPath.c_str();
    VERIFY(k4abt_tracker_create(&sensorCalibration, trackerConfig, &tracker), "Body tracker initialization failed!");

    int depthWidth = sensorCalibration.depth_camera_calibration.resolution_width;
    int depthHeight = sensorCalibration.depth_camera_calibration.resolution_height;

    window3d.Create("3D Visualization", sensorCalibration);
    window3d.SetCloseCallback(CloseCallback);
    window3d.SetKeyCallback(ProcessKey);

    while (playbackResult == K4A_STREAM_RESULT_SUCCEEDED && s_isRunning)
    {
        playbackResult = k4a_playback_get_next_capture(playbackHandle, &capture);
        if (playbackResult == K4A_STREAM_RESULT_EOF)
        {
            // End of file reached
            break;
        }

        if (playbackResult == K4A_STREAM_RESULT_SUCCEEDED)
        {
            // check to make sure we have a depth image
            k4a_image_t depthImage = k4a_capture_get_depth_image(capture);
            if (depthImage == nullptr) {
                //If no depth image, print a warning and skip to next frame
                std::cout << "Warning: No depth image, skipping frame!" << std::endl;
                k4a_capture_release(capture);
                continue;
            }
            // Release the Depth image
            k4a_image_release(depthImage);

            //enque capture and pop results - synchronous
            k4a_wait_result_t queueCaptureResult = k4abt_tracker_enqueue_capture(tracker, capture, K4A_WAIT_INFINITE);

            // Release the sensor capture once it is no longer needed.
            k4a_capture_release(capture);

            if (queueCaptureResult == K4A_WAIT_RESULT_FAILED)
            {
                std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
                break;
            }

            k4abt_frame_t bodyFrame = nullptr;
            k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, K4A_WAIT_INFINITE);
            if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED)
            {
                /************* Successfully get a body tracking result, process the result here ***************/
                VisualizeResult(bodyFrame, window3d, depthWidth, depthHeight); 
                //Release the bodyFrame
                k4abt_frame_release(bodyFrame);
            }
            else
            {
                std::cout << "Pop body frame result failed!" << std::endl;
                break;
            }
        }

        window3d.SetLayout3d(s_layoutMode);
        window3d.SetJointFrameVisualization(s_visualizeJointFrame);
        window3d.Render();
    }

    shutdown_udp();

    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    window3d.Delete();
    printf("Finished body tracking processing!\n");
    k4a_playback_close(playbackHandle);
}
#include <iostream>
#include <sstream>
void PlayFromDevice(InputSettings inputSettings)
{
    uint32_t count = k4a_device_get_installed_count();
    std::stringstream ss;
    ss << "Number of devices detected: ";
    ss << count;
    ss << "!";
    std::string s;
    ss >> s;
    std::cout << s << std::endl;

    // open the device based on its serial number
    vector<string> serial_numbers = { "", "CL2A141004N", "CL2A141004F" };
    cout << "device index: " << inputSettings.device_index << endl;
    k4a_device_t device = nullptr;
    VERIFY(k4a_device_open(inputSettings.device_index, &device), "Open K4A Device failed!");

    // Start camera. Make sure depth camera is enabled.
    k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    deviceConfig.depth_mode = inputSettings.DepthCameraMode;
    deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    VERIFY(k4a_device_start_cameras(device, &deviceConfig), "Start K4A cameras failed!");

    // Get calibration information
    k4a_calibration_t sensorCalibration;
    VERIFY(k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensorCalibration),
        "Get depth camera calibration failed!");
    int depthWidth = sensorCalibration.depth_camera_calibration.resolution_width;
    int depthHeight = sensorCalibration.depth_camera_calibration.resolution_height;

    // Create Body Tracker
    k4abt_tracker_t tracker = nullptr;
    k4abt_tracker_configuration_t trackerConfig = K4ABT_TRACKER_CONFIG_DEFAULT;
    trackerConfig.processing_mode = inputSettings.processingMode;
    trackerConfig.model_path = inputSettings.ModelPath.c_str();
    VERIFY(k4abt_tracker_create(&sensorCalibration, trackerConfig, &tracker), "Body tracker initialization failed!");

    // Initialize the 3d window controller
    Window3dWrapper window3d;
    window3d.Create("3D Visualization", sensorCalibration);
    window3d.SetCloseCallback(CloseCallback);
    window3d.SetKeyCallback(ProcessKey);

    while (s_isRunning)
    {
        k4a_capture_t sensorCapture = nullptr;
        k4a_wait_result_t getCaptureResult = k4a_device_get_capture(device, &sensorCapture, 0); // timeout_in_ms is set to 0

        if (getCaptureResult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            // timeout_in_ms is set to 0. Return immediately no matter whether the sensorCapture is successfully added
            // to the queue or not.
            k4a_wait_result_t queueCaptureResult = k4abt_tracker_enqueue_capture(tracker, sensorCapture, 0);

            // Release the sensor capture once it is no longer needed.
            k4a_capture_release(sensorCapture);

            if (queueCaptureResult == K4A_WAIT_RESULT_FAILED)
            {
                std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
                break;
            }
        }
        else if (getCaptureResult != K4A_WAIT_RESULT_TIMEOUT)
        {
            std::cout << "Get depth capture returned error: " << getCaptureResult << std::endl;
            break;
        }

        // Pop Result from Body Tracker
        k4abt_frame_t bodyFrame = nullptr;
        k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, 0); // timeout_in_ms is set to 0
        if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            /************* Successfully get a body tracking result, process the result here ***************/
            VisualizeResult(bodyFrame, window3d, depthWidth, depthHeight);
            //Release the bodyFrame
            k4abt_frame_release(bodyFrame);
        }
       
        window3d.SetLayout3d(s_layoutMode);
        window3d.SetJointFrameVisualization(s_visualizeJointFrame);
        window3d.Render();
    }

    std::cout << "Finished body tracking processing!" << std::endl;

    shutdown_udp();

    window3d.Delete();
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);

    k4a_device_stop_cameras(device);
    k4a_device_close(device);
}

int main(int argc, char** argv)
{
    InputSettings inputSettings;
   
    if (!ParseInputSettingsFromArg(argc, argv, inputSettings))
    {
        // Print app usage if user entered incorrect arguments.
        PrintUsage();
        return -1;
    }

    // Initialize the UDP socket
    // Check if there are host:port input, otherwise use default
    string ip_address;
    int port;
    int device_index;
    ip_address = !inputSettings.ip_address.empty() ? inputSettings.ip_address : "127.0.0.1";
    port = inputSettings.port ? inputSettings.port : 55555;
    // Set the default device index to 0 if not provided
    if (inputSettings.device_index) {
        cout << "Device index provided: " << inputSettings.device_index << endl;
    }
    else {
        cout << "No device index provided. Using default device index 0." << endl;
        inputSettings.device_index = 0;
    }
    
    setup_udp(ip_address, port);

    // Either play the offline file or play from the device
    if (inputSettings.Offline == true)
    {
        PlayFile(inputSettings);
    }
    else
    {
        PlayFromDevice(inputSettings);
    }

    return 0;
}
