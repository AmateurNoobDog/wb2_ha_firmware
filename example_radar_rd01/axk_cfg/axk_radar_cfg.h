#ifndef _AXK_RADAR_CFG_H_
#define _AXK_RADAR_CFG_H_

#define FUNC_QUEUE_SIZE                 (34)    // n*48 bytes   chirpNum: 34, dataLen: 48
#define FT_QUEUE_SIZE                   (512)   // n*40 bytes   chirpNum: 64, dataLen: 40

#define RAW_DATA_LOG_ENABLE             (0)     // show raw data
#define COMPLETE_DATA_LOG_ENABLE        (0)     // 关闭雷达数据处理日志
#define RADAR_WORK_MODE_LOG_ENABLE      (1)     // show radar operating mode

#define UART_LOG_BAUDRATE               (115200)


// #define DATA_PROCESS_TASK_PRIORITY      (27)
// #define CMD_PROCESS_TASK_PRIORITY       (26)
// #define UART_RECV_CHECK_TASK_PRIORITY   (25)

#define DATA_PROCESS_TASK_PRIORITY      (14)
#define CMD_PROCESS_TASK_PRIORITY       (13)
#define UART_RECV_CHECK_TASK_PRIORITY   (12)

#define TCP_SERVER_PORT                 (9100)

#endif
