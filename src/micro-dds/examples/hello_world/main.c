/** @file main.c
 * @brief Hello World示例 - 基本的DDS发布/订阅演示
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 */

#include "microdds/microdds.h"
#include <stdio.h>
#include <string.h>

/* 示例数据类型 */
typedef struct {
    char message[32];
    uint32_t count;
} HelloWorld_Message;

/* 全局状态 */
static DDS_DomainParticipant g_participant = NULL;
static DDS_Topic g_topic = NULL;
static DDS_Publisher g_publisher = NULL;
static DDS_Subscriber g_subscriber = NULL;
static DDS_DataWriter g_writer = NULL;
static DDS_DataReader g_reader = NULL;

/**
 * @brief 初始化DDS实体
 * @return true 成功
 */
static bool init_dds(void) {
    /* 初始化Micro-DDS库 */
    if (DDS_RETCODE_IS_ERROR(MicroDDS_init())) {
        printf("错误: 无法初始化Micro-DDS\n");
        return false;
    }
    printf("√ Micro-DDS初始化成功 (v%s)\n", MicroDDS_get_version_string());

    /* 创建域参与者 */
    g_participant = DDS_DomainParticipant_create(0, NULL);
    if (g_participant == NULL) {
        printf("错误: 无法创建域参与者\n");
        return false;
    }
    printf("√ 域参与者创建成功 (Domain 0)\n");

    /* 创建主题 */
    g_topic = DDS_Topic_create(g_participant, "HelloWorld", "HelloWorld::Message", NULL);
    if (g_topic == NULL) {
        printf("错误: 无法创建主题\n");
        return false;
    }
    printf("√ 主题创建成功: '%s' (type: '%s')\n",
           DDS_Topic_get_name(g_topic),
           DDS_Topic_get_type_name(g_topic));

    /* 创建发布者 */
    g_publisher = DDS_Publisher_create(g_participant, NULL);
    if (g_publisher == NULL) {
        printf("错误: 无法创建发布者\n");
        return false;
    }
    printf("√ 发布者创建成功\n");

    /* 创建订阅者 */
    g_subscriber = DDS_Subscriber_create(g_participant, NULL);
    if (g_subscriber == NULL) {
        printf("错误: 无法创建订阅者\n");
        return false;
    }
    printf("√ 订阅者创建成功\n");

    /* 创建数据写入器 */
    g_writer = DDS_DataWriter_create(g_publisher, g_topic, NULL);
    if (g_writer == NULL) {
        printf("错误: 无法创建数据写入器\n");
        return false;
    }
    printf("√ 数据写入器创建成功\n");

    /* 创建数据读取器 */
    g_reader = DDS_DataReader_create(g_subscriber, g_topic, NULL);
    if (g_reader == NULL) {
        printf("错误: 无法创建数据读取器\n");
        return false;
    }
    printf("√ 数据读取器创建成功\n");

    return true;
}

/**
 * @brief 清理DDS实体
 */
static void cleanup_dds(void) {
    if (g_reader != NULL) {
        (void)DDS_DataReader_delete(g_reader);
        printf("√ 数据读取器已删除\n");
    }

    if (g_writer != NULL) {
        (void)DDS_DataWriter_delete(g_writer);
        printf("√ 数据写入器已删除\n");
    }

    if (g_subscriber != NULL) {
        (void)DDS_Subscriber_delete(g_subscriber);
        printf("√ 订阅者已删除\n");
    }

    if (g_publisher != NULL) {
        (void)DDS_Publisher_delete(g_publisher);
        printf("√ 发布者已删除\n");
    }

    if (g_topic != NULL) {
        (void)DDS_Topic_delete(g_topic);
        printf("√ 主题已删除\n");
    }

    if (g_participant != NULL) {
        (void)DDS_DomainParticipant_delete(g_participant);
        printf("√ 域参与者已删除\n");
    }

    (void)MicroDDS_shutdown();
    printf("√ Micro-DDS已关闭\n");
}

/**
 * @brief 模拟发送消息
 */
static void send_messages(uint32_t count) {
    HelloWorld_Message msg;

    printf("\n--- 发送消息 ---\n");

    for (uint32_t i = 0U; i < count; i++) {
        /* 构造消息 */
        (void)snprintf(msg.message, sizeof(msg.message), "Hello World %u", i + 1U);
        msg.count = i + 1U;

        /* 发送消息 */
        DDS_ReturnCode_t rc = DDS_DataWriter_write(g_writer, &msg, DDS_HANDLE_NIL);

        if (DDS_RETCODE_IS_OK(rc)) {
            printf("发送: '%s' (count=%u)\n", msg.message, msg.count);
        } else {
            printf("错误: 发送失败 (rc=%d)\n", rc);
        }
    }
}

/**
 * @brief 主函数
 */
int main(void) {
    printf("========================================\n");
    printf("  Micro-DDS Hello World 示例\n");
    printf("========================================\n\n");

    /* 初始化 */
    if (!init_dds()) {
        cleanup_dds();
        return 1;
    }

    printf("\n--- DDS实体创建完成 ---\n");

    /* 发送消息 */
    send_messages(5U);

    printf("\n--- 示例完成 ---\n");

    /* 清理 */
    cleanup_dds();

    return 0;
}
