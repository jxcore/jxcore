// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_JOB_H_
#define SRC_JX_JOB_H_

namespace jxcore {
void SendMessage(const int threadId, const char *msg_data, const int length,
                 bool same_thread);

int CreateInstances(const int count);

void CreateThread(void (*entry)(void *arg), void *param);
}

#endif  // SRC_JX_JOB_H_
