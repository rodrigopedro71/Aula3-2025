#include "rr.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>
#include "queue.h"
#include <stdint.h>

/**
 * @brief First-In-First-Out (FIFO) scheduling algorithm.
 *
 * This function implements the FIFO scheduling algorithm. If the CPU is not idle it
 * checks if the application is ready and frees the CPU.
 * If the CPU is idle, it selects the next task to run based on the order they were added
 * to the ready queue. The task that has been in the queue the longest is selected to run next.
 *
 * @param current_time_ms The current time in milliseconds.
 * @param rq Pointer to the ready queue containing tasks that are ready to run.
 * @param cpu_task Double pointer to the currently running task. This will be updated
 *                 to point to the next task to run.
 */



pcb_t* aplicarRoundRobin(queue_t *rq, pcb_t *cpu_task, uint32_t current_time_ms) {
    if (cpu_task && (current_time_ms - cpu_task->slice_start_ms) >= 500
        && cpu_task->ellapsed_time_ms < cpu_task->time_ms) {
        enqueue_pcb(rq, cpu_task);   // envia para o fim da fila
        cpu_task = NULL;
    }

    if (cpu_task == NULL) {          // CPU livre: busca próximo
        cpu_task = dequeue_pcb(rq);
        if (cpu_task) cpu_task->slice_start_ms = current_time_ms;
    }

    return cpu_task;
}



void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;      // Add to the running time of the application/task
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task finished
            // Send msg to application
            msg_t msg = {
                    .pid = (*cpu_task)->pid,
                    .request = PROCESS_REQUEST_DONE,
                    .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        }
        else if ((current_time_ms - (*cpu_task)->slice_start_ms) >= 500) {
            (*cpu_task)->slice_start_ms = current_time_ms;
            enqueue_pcb(rq, *cpu_task);
            *cpu_task = NULL;
        }
    }
    if (*cpu_task == NULL && rq->head != NULL) {
        *cpu_task = aplicarRoundRobin(rq, *cpu_task, TICKS_MS);
        (*cpu_task)->slice_start_ms = current_time_ms;
    }
}
