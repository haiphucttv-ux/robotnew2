#include "robot_microros.h"

#ifdef ROBOT_MICROROS
#include "cmsis_os.h"
#include "lwip/sockets.h"
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rcutils/allocator.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/u_int32.h>
#include <uxr/client/transport.h>

#ifndef ROBOT_MICROROS_AGENT_IP
#define ROBOT_MICROROS_AGENT_IP "192.168.1.100"
#endif

extern void *microros_allocate(size_t size, void *state);
extern void microros_deallocate(void *pointer, void *state);
extern void *microros_reallocate(void *pointer, size_t size, void *state);
extern void *microros_zero_allocate(size_t count, size_t size, void *state);

#define AGENT_PORT 8888
static int agent_socket = -1;

static bool Udp_Close(struct uxrCustomTransport *transport)
{
    (void)transport;
    if (agent_socket >= 0)
    {
        lwip_close(agent_socket);
        agent_socket = -1;
    }
    return true;
}

static bool Udp_Open(struct uxrCustomTransport *transport)
{
    const struct sockaddr_in agent = {
        .sin_family = AF_INET,
        .sin_port = lwip_htons(AGENT_PORT),
        .sin_addr.s_addr = ipaddr_addr((const char *)transport->args)
    };

    agent_socket = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (agent_socket < 0)
        return false;

    if (lwip_connect(agent_socket, (const struct sockaddr *)&agent, sizeof(agent)) < 0)
    {
        Udp_Close(transport);
        return false;
    }
    return true;
}

static size_t Udp_Write(struct uxrCustomTransport *transport,
                        const uint8_t *buffer, size_t length, uint8_t *error)
{
    (void)transport;
    int result = lwip_send(agent_socket, buffer, length, 0);
    *error = result < 0;
    return result > 0 ? (size_t)result : 0U;
}

static size_t Udp_Read(struct uxrCustomTransport *transport,
                       uint8_t *buffer, size_t length, int timeout, uint8_t *error)
{
    (void)transport;
    *error = 0;
    if (timeout <= 0)
    {
        int result = lwip_recv(agent_socket, buffer, length, MSG_DONTWAIT);
        return result > 0 ? (size_t)result : 0U;
    }

    struct timeval receive_timeout = { timeout / 1000, (timeout % 1000) * 1000 };
    if (lwip_setsockopt(agent_socket, SOL_SOCKET, SO_RCVTIMEO,
                        &receive_timeout, sizeof(receive_timeout)) < 0)
    {
        *error = 1;
        return 0;
    }

    int result = lwip_recv(agent_socket, buffer, length, 0);
    return result > 0 ? (size_t)result : 0U;
}

static void Publish_Heartbeat(std_msgs__msg__UInt32 *heartbeat)
{
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support = {0};
    rcl_node_t node = rcl_get_zero_initialized_node();
    rcl_publisher_t publisher = rcl_get_zero_initialized_publisher();

    if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK)
    {
        Udp_Close(NULL);
        return;
    }

    if (rclc_node_init_default(&node, "robotnew2_stm32", "", &support) == RCL_RET_OK)
    {
        if (rclc_publisher_init_default(&publisher, &node,
                ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt32),
                "/robotnew2/heartbeat") == RCL_RET_OK)
        {
            while (rcl_publish(&publisher, heartbeat, NULL) == RCL_RET_OK)
            {
                ++heartbeat->data;
                osDelay(1000U);
                if (rmw_uros_ping_agent(100, 1) != RMW_RET_OK)
                    break;
            }
            rmw_uros_set_context_entity_destroy_session_timeout(
                rcl_context_get_rmw_context(&support.context), 0);
            (void)rcl_publisher_fini(&publisher, &node);
        }
        (void)rcl_node_fini(&node);
    }
    (void)rclc_support_fini(&support);
}

void Robot_MicroRosTask(void const *argument)
{
    (void)argument;
    static char agent_ip[] = ROBOT_MICROROS_AGENT_IP;
    std_msgs__msg__UInt32 heartbeat = {0};

    rcutils_allocator_t allocator = rcutils_get_zero_initialized_allocator();
    allocator.allocate = microros_allocate;
    allocator.deallocate = microros_deallocate;
    allocator.reallocate = microros_reallocate;
    allocator.zero_allocate = microros_zero_allocate;

    if (!rcutils_set_default_allocator(&allocator) ||
        rmw_uros_set_custom_transport(false, agent_ip,
            Udp_Open, Udp_Close, Udp_Write, Udp_Read) != RMW_RET_OK)
    {
        (void)osThreadTerminate(osThreadGetId());
        return;
    }

    for (;;)
    {
        if (rmw_uros_ping_agent(100, 1) == RMW_RET_OK)
            Publish_Heartbeat(&heartbeat);

        rcl_reset_error();
        osDelay(1000U);
    }
}
#endif
