/******************************************************************************
 * @file    media_controller.c
 * @brief   Media/Entertainment Controller Publisher
 *
 * Publishes media playback status
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "infotainment_types.h"
#include "../../src/dds/core/dds_core.h"

#define MEDIA_TOPIC_NAME    "Infotainment/Media/Status"
#define MEDIA_TYPE_NAME     "MediaStatusType"
#define MEDIA_DOMAIN_ID     4
#define MEDIA_PUBLISH_HZ    10

static volatile bool g_running = true;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Media Controller] Shutting down...\n");
}

static const char* tracks[] = {
    "Bohemian Rhapsody",
    "Stairway to Heaven",
    "Hotel California",
    "Sweet Child O' Mine",
    "Imagine"
};

static const char* artists[] = {
    "Queen",
    "Led Zeppelin",
    "Eagles",
    "Guns N' Roses",
    "John Lennon"
};

static void generate_media_data(MediaStatusType* media)
{
    static uint32_t trackIndex = 0;
    static uint32_t position = 0;
    
    media->mediaId = 0xD003;
    media->source = MEDIA_SOURCE_STREAMING;
    media->state = MEDIA_STATE_PLAYING;
    
    /* Track info */
    trackIndex = (position / 300) % 5;
    strcpy(media->title, tracks[trackIndex]);
    strcpy(media->artist, artists[trackIndex]);
    strcpy(media->album, "Greatest Hits");
    media->trackNumber = trackIndex + 1;
    media->totalTracks = 5;
    media->durationSec = 240 + trackIndex * 30;
    
    /* Playback */
    media->currentPositionSec = (position % media->durationSec);
    position++;
    media->volumePercent = 65;
    media->muted = false;
    media->balance = 50;
    media->fade = 50;
    
    /* Audio settings */
    media->bassLevel = 2;
    media->trebleLevel = 1;
    media->midLevel = 0;
    
    /* Modes */
    media->shuffleEnabled = false;
    media->repeatMode = REPEAT_MODE_ALL;
    
    /* Streaming specific */
    media->albumArtAvailable = true;
    media->albumArtSize = 50000;
    
    media->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Media Controller\n");
    printf("  QM (Non-Safety Critical)\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (dds_runtime_init() != ETH_OK) {
        printf("[ERROR] DDS init failed\n");
        return 1;
    }
    
    dds_domain_participant_t* participant = dds_create_participant(MEDIA_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_BEST_EFFORT, DDS_QOS_DURABILITY_VOLATILE, 150, 10, 2 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, MEDIA_TOPIC_NAME, MEDIA_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Media Controller] Running...\n");
    
    MediaStatusType media;
    uint32_t count = 0;
    
    while (g_running) {
        generate_media_data(&media);
        
        if (dds_write(writer, &media, sizeof(media), 50) == ETH_OK) {
            if (++count % 10 == 0) {
                printf("[Media] %s - %s (%d:%02d / %d:%02d)\n",
                       media.artist, media.title,
                       media.currentPositionSec / 60, media.currentPositionSec % 60,
                       media.durationSec / 60, media.durationSec % 60);
            }
        }
        usleep(1000000 / MEDIA_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    dds_runtime_deinit();
    
    return 0;
}
