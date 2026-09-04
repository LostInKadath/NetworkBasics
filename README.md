# Senior C++ Networking / Linux Performance — Step-by-Step Learning Plan

Цель плана — подготовиться к Senior C++ позициям в области high-performance networking, kernel bypass, Linux internals и low-latency systems (например, роли вокруг ConnectX/BlueField, DPDK, RDMA, высокопроизводительных TCP/IP stacks).

План рассчитан примерно на **6–8 недель при 1–2 часах в день**. Его можно проходить быстрее, если часть тем уже знакома.

## Как работать с планом

Для каждого этапа используй цикл:

1. Изучить концепцию.
2. Объяснить её вслух без шпаргалки.
3. Написать маленький проект.
4. Снять измерения / трассировку / профили.
5. Ответить на вопросы интервью.
6. Только после этого переходить дальше.

### Уровень готовности этапа

- **Green** — можешь объяснить концепцию и ответить на типовые вопросы.
- **Yellow** — понимаешь идею, но не можешь уверенно объяснить детали.
- **Red** — узнаёшь термин, но не можешь связать его с реальным поведением системы.

Для Senior-интервью цель — Green по темам 🔴, уверенный Yellow/Green по 🟠 и базовый Yellow по 🟡.

---

# Stage 0 — Baseline: Linux, C++ and tooling

**Приоритет:** 🔴  
**Цель:** убедиться, что инструменты для дальнейших этапов не создают лишнего шума.

## Что изучить

- Linux processes / threads.
- File descriptors.
- `read`, `write`, `recv`, `send`.
- `socket`, `bind`, `listen`, `accept`, `connect`.
- `errno`.
- `strace`.
- `gdb`.
- `perf` basics.
- `tcpdump` basics.
- `ip` command:
  - `ip addr`
  - `ip route`
  - `ip neigh`
  - `ip link`
- `ss`.

## Практика

### Mini-project: TCP Echo Server

Написать C++ TCP echo server:

- IPv4;
- multiple clients;
- blocking version;
- non-blocking version;
- затем добавить `epoll`.

Сравнить три реализации:
1. blocking;
2. one-thread-per-connection;
3. `epoll`.

## Типовые вопросы

- Что такое file descriptor?
- Что возвращает `socket()`?
- Что делает `bind()`?
- Зачем нужен `listen()`?
- Почему `accept()` возвращает новый socket?
- Чем listening socket отличается от connected socket?
- Что происходит после `connect()`?
- Что означает `recv() == 0`?
- Может ли `send()` отправить меньше байт?
- Почему `write()` может вернуть `EAGAIN`?
- Чем `errno` отличается от return value?
- Что делает `strace`?
- Что можно узнать через `ss`?

## Definition of Done

Ты можешь нарисовать путь:

```text
application
    |
socket API
    |
kernel
    |
TCP/IP
    |
NIC driver
    |
NIC
```

и объяснить, что происходит при `connect()` и `send()`.

---

# Stage 1 — TCP/IP Fundamentals

**Приоритет:** 🔴  
**Цель:** перейти от "я пользовался sockets" к пониманию TCP/IP.

## Что изучить

### Layering

- Ethernet.
- ARP.
- IPv4.
- ICMP.
- TCP.
- UDP.
- ports.
- routing.

### Packet structure

Понимать поля:

- Ethernet header;
- IPv4 header;
- TCP header;
- UDP header.

Особенно:

- source/destination;
- protocol;
- TTL;
- checksum;
- sequence number;
- acknowledgement number;
- flags.

### Routing

- routing table;
- default route;
- longest prefix match;
- gateway;
- local subnet.

### MTU

- MTU;
- fragmentation;
- Path MTU Discovery;
- MSS.

## Практика

### Mini-project: Packet Inspector

Используя `AF_PACKET` или `libpcap`, сделать небольшой packet sniffer:

- Ethernet;
- IPv4;
- TCP/UDP;
- вывести addresses/ports;
- TCP flags;
- packet size;
- sequence / ACK numbers.

Добавить фильтр:

```text
TCP dst port 8080
```

## Типовые вопросы

- Чем frame отличается от packet и segment?
- Что делает ARP?
- Как host решает, отправлять packet напрямую или через gateway?
- Что происходит при обращении к IP за пределами local subnet?
- Что такое MTU?
- Почему TCP MSS обычно меньше MTU?
- Что происходит при превышении MTU?
- Что такое fragmentation?
- Зачем нужен TTL?
- Где вычисляется IP checksum?
- Что произойдет, если packet потерян?

## Definition of Done

По `tcpdump` ты можешь вручную объяснить handshake:

```text
SYN
SYN-ACK
ACK
```

и каждую последующую пару:

```text
SEQ
ACK
LEN
```

---

# Stage 2 — TCP Internals

**Приоритет:** 🔴  
**Цель:** понимать TCP как механизм доставки данных, а не как "надёжный поток".

## Что изучить

- byte stream;
- sequence numbers;
- ACK;
- sliding window;
- receive window;
- send window;
- flow control;
- congestion control;
- retransmission;
- RTT;
- RTO;
- duplicate ACK;
- fast retransmit;
- cumulative ACK;
- out-of-order packets;
- delayed ACK;
- Nagle algorithm;
- `TCP_NODELAY`;
- keepalive;
- half-close;
- TCP states;
- TIME_WAIT.

## Практика

### Mini-project: TCP State / Sequence Visualizer

Собрать небольшой анализатор `pcap`, который:

- показывает TCP handshake;
- строит timeline SEQ/ACK;
- обнаруживает retransmissions;
- показывает out-of-order packets;
- показывает FIN/RST;
- строит TCP connection state transitions.

Дополнительно:
- искусственно добавить packet loss с помощью `tc netem`;
- сравнить поведение TCP до/после потерь.

Пример:

```bash
tc qdisc add dev lo root netem loss 1%
```

## Типовые вопросы

- Почему TCP — stream, а не message protocol?
- Почему один `send()` может соответствовать нескольким `recv()`?
- Может ли один TCP segment содержать данные от нескольких `send()`?
- Что происходит при потере TCP segment?
- Чем flow control отличается от congestion control?
- Что такое receive window?
- Что такое congestion window?
- Что вызывает retransmission?
- Что такое RTO?
- Что такое fast retransmit?
- Зачем нужен sequence number?
- Зачем нужен ACK number?
- Почему TCP имеет TIME_WAIT?
- Почему нельзя просто сразу закрыть connection?
- Что делает `TCP_NODELAY`?
- Когда Nagle полезен, а когда вреден?
- Что делает `shutdown(SHUT_WR)`?
- Чем FIN отличается от RST?

## Definition of Done

Ты способен объяснить, почему приложение может иметь:

```text
application writes 100 KB
```

а на wire получится:

```text
many TCP segments
```

и почему peer может получить эти данные через любое количество `recv()`.

---

# Stage 3 — Linux Socket Programming and Multiplexing

**Приоритет:** 🔴  
**Цель:** уверенно знать Linux networking API.

## Что изучить

- blocking sockets;
- non-blocking sockets;
- `fcntl`;
- `O_NONBLOCK`;
- `select`;
- `poll`;
- `epoll`;
- level-triggered;
- edge-triggered;
- `EPOLLIN`;
- `EPOLLOUT`;
- `EPOLLERR`;
- `EPOLLHUP`;
- accept loops;
- read loops;
- partial writes;
- backpressure;
- socket buffers.

## Практика

### Mini-project: Event-driven TCP Server

Расширить Stage 0:

- non-blocking sockets;
- `epoll`;
- ET mode;
- proper drain-until-`EAGAIN`;
- output queue;
- partial writes;
- per-client state;
- connection timeout;
- graceful shutdown.

Добавить benchmark:

```text
clients
messages/sec
throughput
p50 latency
p99 latency
CPU utilization
```

## Типовые вопросы

- Почему ET требует non-blocking I/O?
- Когда нужно снова вызывать `epoll_wait()`?
- Почему read loop должен идти до `EAGAIN`?
- Что произойдет, если не дочитать socket в ET mode?
- Когда возникает `EPOLLOUT`?
- Почему нельзя постоянно подписываться на `EPOLLOUT`?
- Что делать при partial write?
- Как организовать per-connection output buffer?
- Что происходит с data, когда socket send buffer переполнен?
- Где возникает backpressure?
- Что будет при медленном клиенте?
- Как защитить event loop от одного "тяжёлого" клиента?

## Definition of Done

Ты можешь на доске написать корректный ET event loop и объяснить каждую проверку.

---

# Stage 4 — Linux Networking Internals

**Приоритет:** 🔴  
**Цель:** понять путь packet внутри Linux.

## Что изучить

- socket layer;
- syscall boundary;
- sk_buff (`skb`) концептуально;
- TCP/IP kernel stack;
- NIC driver;
- interrupts;
- NAPI;
- softirq;
- packet receive path;
- packet transmit path;
- socket buffers;
- network namespaces;
- `veth`;
- bridges;
- routing namespaces.

## Практика

### Mini-project: Trace a Packet Through Linux

Создать network namespace topology:

```text
ns1
 |
veth
 |
router namespace
 |
veth
 |
ns2
```

Настроить routing вручную.

Затем:

- `tcpdump`;
- `ss`;
- `ip route`;
- `ip neigh`;
- `strace`.

Документировать полный путь packet:

```text
application
 -> syscall
 -> kernel socket
 -> TCP
 -> IP
 -> routing
 -> qdisc
 -> driver
 -> NIC
```

## Типовые вопросы

- Где находится TCP stack?
- Что делает kernel после `send()`?
- Что происходит с packet после получения NIC?
- Что такое interrupt?
- Что такое NAPI?
- Почему networking использует softirq?
- Что такое `skb`?
- Где находится socket receive buffer?
- Когда kernel копирует данные в userspace?
- Где возникают copies?
- Какие части networking выполняются на CPU?
- Что происходит при receive buffer overflow?

## Definition of Done

Ты можешь подробно объяснить путь одного входящего TCP packet от NIC до `recv()`.

---

# Stage 5 — Zero-copy and Memory

**Приоритет:** 🟠  
**Цель:** понять, почему memory movement часто является bottleneck.

## Что изучить

- copy vs zero-copy;
- `read`/`recv` copies;
- `sendfile`;
- `splice`;
- `mmap`;
- DMA;
- page pinning;
- page faults;
- huge pages;
- cache locality;
- memory bandwidth.

## Практика

### Mini-project: Copy Benchmark

Сделать benchmark нескольких путей передачи большого буфера:

1. `read()` + `write()`;
2. `mmap()`;
3. `sendfile()`;
4. `splice()` где применимо.

Измерять:

- throughput;
- CPU;
- cycles/byte;
- memory bandwidth.

Инструменты:

```text
perf
strace
time
```

## Типовые вопросы

- Где происходят memory copies при обычном TCP?
- Почему copy expensive?
- Что такое DMA?
- Почему DMA не означает "CPU вообще не участвует"?
- Что такое zero-copy?
- Что делает `mmap()`?
- Что такое page fault?
- Зачем huge pages?
- Что такое pinned memory?
- Как CPU cache влияет на networking performance?

---

# Stage 6 — CPU, Cache, NUMA and Low Latency

**Приоритет:** 🟠  
**Цель:** перейти от "fast" к осмысленной performance engineering.

## Что изучить

- CPU cache hierarchy;
- cache line;
- cache miss;
- cache coherence;
- false sharing;
- branch prediction;
- memory ordering;
- atomics;
- lock contention;
- CPU affinity;
- CPU isolation concept;
- NUMA;
- local vs remote memory;
- tail latency;
- p50/p95/p99/p99.9.

## Практика

### Mini-project: Low-latency Queue Benchmark

Сделать producer/consumer benchmark:

1. mutex + queue;
2. lock-free SPSC ring buffer;
3. multiple producers / consumers.

Измерять:

- throughput;
- average latency;
- p99;
- p99.9;
- CPU utilization.

Затем:

- pin threads to CPUs;
- проверить effect of CPU affinity;
- проверить false sharing;
- проверить NUMA, если доступна multi-socket система.

## Типовые вопросы

- Почему average latency может быть хорошей, а p99 плохим?
- Что такое false sharing?
- Почему cache line обычно важнее размера C++ объекта?
- Что такое cache coherence?
- Почему spinlock может быть быстрее mutex?
- Когда spinlock хуже?
- Что такое memory ordering?
- Что такое NUMA?
- Почему remote memory access дороже?
- Зачем CPU affinity?
- Почему throughput может расти, а latency ухудшаться?
- Что ты будешь измерять первым при performance problem?

## Definition of Done

Ты можешь предложить методику поиска bottleneck, а не просто сказать "профилировать".

---

# Stage 7 — Kernel Bypass

**Приоритет:** 🔴  
**Цель:** понять основную идею вакансии NVIDIA.

## Что изучить

- kernel bypass;
- userspace networking;
- polling;
- DMA;
- descriptor rings;
- RX/TX queues;
- memory registration;
- huge pages;
- NIC queues;
- batching;
- busy polling;
- zero-copy.

Понимать:

```text
Traditional:

Application
    |
syscall
    |
kernel TCP/IP
    |
driver
    |
NIC


Kernel bypass:

Application
    |
userspace stack
    |
NIC
```

## Практика

### Mini-project: User-space Packet I/O

Использовать DPDK.

Сделать:

```text
NIC -> userspace -> packet parser -> userspace -> NIC
```

Минимум:

- initialize EAL;
- receive packets;
- parse Ethernet/IPv4/UDP;
- count packets/bytes;
- transmit packets back;
- measure Mpps and CPU usage.

После этого добавить:

- batching;
- multiple RX queues;
- CPU pinning.

## Типовые вопросы

- Почему kernel bypass быстрее?
- Какие overheads убираются?
- Почему polling быстрее interrupt-driven I/O для low latency?
- Почему polling не всегда выгоден?
- Зачем huge pages?
- Что такое DMA?
- Что такое descriptor ring?
- Что такое RX/TX queue?
- Где хранится packet buffer?
- Как NIC узнаёт, где лежит buffer?
- Как application узнаёт, что packet пришел?
- Что такое batching?
- Почему batching увеличивает throughput, но может увеличить latency?
- Что делает DPDK EAL?

## Definition of Done

Ты можешь объяснить архитектурно:

```text
CPU
 |
PCIe
 |
NIC
 | \
RX  TX queues
 |    |
DMA  DMA
 \    /
userspace
```

---

# Stage 8 — Build a Minimal User-space UDP Stack

**Приоритет:** 🔴  
**Цель:** закрепить networking на уровне packet processing.

## Что изучить

- raw Ethernet;
- ARP;
- IPv4;
- UDP;
- checksums;
- routing concept;
- packet buffers.

## Практика

### Mini-project: Userspace UDP

Сделать минимальный userspace stack:

```text
Ethernet
   |
ARP
   |
IPv4
   |
UDP
   |
Application
```

Функциональность:

- parse Ethernet;
- ARP request/reply;
- IPv4;
- UDP receive;
- UDP transmit;
- checksum;
- packet counters.

Не пытаться сразу реализовывать полноценный TCP.

## Типовые вопросы

- Как написать UDP packet вручную?
- Что такое Ethernet destination MAC?
- Как узнать MAC следующего hop?
- Что происходит при необходимости отправить packet через gateway?
- Как рассчитывается UDP checksum?
- Где находится ARP cache?
- Чем L2 address отличается от L3 address?
- Как routing и ARP работают вместе?

---

# Stage 9 — DPDK Deep Dive

**Приоритет:** 🟠  
**Цель:** понимать, как строятся production-grade high-speed packet pipelines.

## Что изучить

- DPDK mempool;
- mbuf;
- PMD;
- poll mode driver;
- RX/TX bursts;
- multi-queue;
- RSS;
- NUMA awareness;
- lcore;
- huge pages;
- port/queue configuration.

## Практика

### Mini-project: High-speed UDP Router

DPDK application:

```text
RX queue
   |
parse
   |
route by UDP port
   |
TX queue
```

Добавить:

- multiple queues;
- per-core processing;
- CPU affinity;
- packet batching;
- counters;
- latency measurement.

## Типовые вопросы

- Что такое mbuf?
- Что такое mempool?
- Почему packet allocation нельзя делать через `malloc()` на каждый packet?
- Что такое PMD?
- Почему DPDK polling?
- Как работает RSS?
- Как распределить traffic между cores?
- Что такое RX/TX queue affinity?
- Как NUMA влияет на DPDK?
- Почему one queue per core часто полезна?
- Где bottleneck может появиться после увеличения числа cores?

---

# Stage 10 — RDMA / RoCE

**Приоритет:** 🟠  
**Цель:** получить понимание второго большого направления NVIDIA networking.

## Что изучить

- RDMA;
- verbs;
- QP;
- CQ;
- completion;
- MR;
- memory registration;
- send/receive;
- RDMA Read;
- RDMA Write;
- RoCE;
- lossless Ethernet concept;
- PFC;
- ECN.

## Практика

### Mini-project: RDMA Ping-Pong

На двух Linux machines или VM environment, где RDMA доступно:

- client/server;
- RC QP;
- send/receive;
- измерить latency;
- сравнить с TCP socket ping-pong.

Результат оформить в таблицу:

```text
TCP:
avg
p99

RDMA:
avg
p99
```

Если реальное RDMA hardware недоступно, сначала изучить verbs и архитектуру, а hands-on заменить небольшой симуляцией API/benchmark design.

## Типовые вопросы

- Что такое RDMA?
- Почему RDMA может иметь низкую latency?
- Что делает memory registration?
- Что такое QP?
- Что такое CQ?
- Чем RDMA Write отличается от Send?
- Почему remote memory access требует специальных механизмов?
- Что такое RoCE?
- Что такое PFC?
- Что такое ECN?
- Чем RDMA отличается от обычного TCP/IP networking?

---

# Stage 11 — NIC / PCIe / ConnectX / BlueField

**Приоритет:** 🟡 → 🟠  
**Цель:** привязать изученные concepts к NVIDIA hardware.

## Что изучить

### PCIe

- PCIe hierarchy;
- root complex;
- endpoint;
- lanes;
- DMA;
- BAR;
- MSI/MSI-X.

### NIC

- queues;
- descriptors;
- completion queues;
- DMA;
- RSS;
- offloads;
- hardware timestamping.

### NVIDIA ecosystem

На концептуальном уровне:

- ConnectX;
- BlueField DPU;
- RDMA/RoCE;
- DOCA;
- hardware offload.

## Практика

### Mini-project: NIC Architecture Notebook

Сделать техническую заметку на 2–4 страницы:

```text
Application
 |
DPDK / RDMA / DOCA
 |
userspace libraries
 |
PCIe
 |
ConnectX
 |
Ethernet network
```

Для каждого блока указать:

- кто управляет ресурсом;
- где находится state;
- где выполняется computation;
- где происходит DMA;
- где possible bottlenecks.

## Типовые вопросы

- Как NIC получает доступ к RAM?
- Что такое DMA?
- Что такое PCIe BAR?
- Что такое MSI-X?
- Зачем NIC много queues?
- Что такое RSS?
- Какие операции можно offload в NIC?
- Что такое SmartNIC / DPU?
- Чем DPU отличается от NIC?
- Какие задачи имеет смысл offload в hardware?

---

# Stage 12 — Linux Kernel / Driver Basics

**Приоритет:** 🟠  
**Цель:** соответствовать требованию "Linux user space/driver/kernel development".

## Что изучить

- kernel/user-space boundary;
- syscalls;
- kernel modules;
- character devices;
- `ioctl`;
- `mmap`;
- interrupts;
- DMA;
- device memory;
- PCI devices;
- sysfs;
- `/proc`;
- `/sys`.

## Практика

### Mini-project: Tiny Linux Device

Написать простой kernel module:

- module init/exit;
- character device;
- `open/read/write/ioctl`;
- userspace client.

Затем добавить:

- `mmap` или conceptual design;
- простой shared buffer.

Цель проекта — не писать настоящий production driver, а понять boundary:

```text
userspace
    |
syscall/ioctl
    |
kernel
    |
device
```

## Типовые вопросы

- Почему userspace не может просто обратиться к hardware?
- Что делает `ioctl()`?
- Чем `mmap()` в kernel context отличается от обычного userspace mapping?
- Что такое interrupt handler?
- Что такое bottom half / deferred work concept?
- Почему kernel code не может использовать обычный userspace pointer без проверки?
- Что такое DMA mapping?
- Как driver сообщает userspace о событии?

---

# Stage 13 — Performance Investigation

**Приоритет:** 🔴  
**Цель:** научиться проводить performance investigation как Senior Engineer.

## Что изучить

- benchmarking methodology;
- warm-up;
- steady state;
- measurement overhead;
- CPU profiling;
- hardware counters;
- latency histograms;
- throughput;
- saturation;
- queueing;
- Little's Law на концептуальном уровне.

Инструменты:

- `perf stat`;
- `perf record`;
- `perf report`;
- `flamegraph`;
- `strace`;
- `tcpdump`;
- `ss`;
- `sar`;
- `vmstat`;
- `iostat`;
- `numactl`;
- `taskset`.

## Практика

### Mini-project: Find a Hidden Bottleneck

Взять один из предыдущих проектов и специально внести 3 bottlenecks:

1. lock contention;
2. unnecessary memory copy;
3. poor CPU affinity.

Затем провести investigation только по observable symptoms:

```text
throughput ↓
p99 latency ↑
CPU ↑
```

Найти причины с помощью профилирования.

## Типовые вопросы

- С чего ты начинаешь performance investigation?
- Почему нельзя сразу оптимизировать код?
- Как отличить CPU bottleneck от memory bottleneck?
- Как найти lock contention?
- Как измерить tail latency?
- Как доказать, что optimization действительно помогла?
- Почему benchmark может лгать?
- Что такое warm cache / cold cache?
- Почему debug build нельзя использовать для performance conclusions?
- Как performance изменяется при увеличении concurrency?

---

# Stage 14 — Final Project: Mini High-Performance Networking Stack

**Приоритет:** 🔴  
**Цель:** собрать все знания в один проект.

## Проект

### "Userspace High-Performance UDP/TCP Gateway"

Архитектура:

```text
                 +----------------+
NIC ------------>| RX             |
                 |                |
                 | Packet Parser  |
                 |      |         |
                 | Routing       |
                 |      |         |
                 | Protocol      |
                 |      |         |
                 | TX             |
                 +----------------+
```

### Phase 1

UDP forwarding:

```text
NIC A -> userspace -> NIC B
```

### Phase 2

Добавить:

- RSS;
- multiple queues;
- CPU affinity;
- batching;
- per-core statistics.

### Phase 3

Добавить latency measurement:

```text
p50
p95
p99
p99.9
```

### Phase 4

Сделать две реализации:

```text
Linux sockets
vs
DPDK
```

Сравнить:

- throughput;
- packets/sec;
- CPU;
- latency;
- tail latency.

### Phase 5

Написать design document:

- architecture;
- bottlenecks;
- scalability;
- failure modes;
- NUMA;
- backpressure;
- queueing;
- observability;
- trade-offs.

Это уже можно использовать как основу для обсуждения на интервью.

---

# Interview Question Bank

## C++ / Concurrency

1. Чем mutex отличается от spinlock?
2. Когда lock-free структура действительно быстрее?
3. Что такое memory ordering?
4. Что такое false sharing?
5. Почему atomic operation может быть дорогой?
6. Как определить data race?
7. Как избежать contention?
8. Как устроить bounded MPMC queue?
9. Почему memory allocation может стать bottleneck?
10. Как уменьшить allocation rate в hot path?

## Linux

1. Что происходит при system call?
2. Чем process отличается от thread?
3. Что происходит при context switch?
4. Что такое virtual memory?
5. Что такое page fault?
6. Что делает `mmap()`?
7. Что такое `epoll`?
8. Чем ET отличается от LT?
9. Что такое NAPI?
10. Как application взаимодействует с kernel driver?

## TCP/IP

1. Как устанавливается TCP connection?
2. Как TCP обнаруживает потерю packet?
3. Flow control vs congestion control?
4. Что такое MSS?
5. Что такое MTU?
6. Что такое retransmission timeout?
7. Что такое TIME_WAIT?
8. Что делает SYN backlog?
9. Что такое receive queue?
10. Как TCP превращает unreliable IP into reliable stream?

## Networking Performance

1. Почему kernel bypass быстрее?
2. Почему polling быстрее interrupts?
3. Когда interrupts лучше polling?
4. Почему batching увеличивает throughput?
5. Почему batching может увеличить latency?
6. Что такое zero-copy?
7. Где происходят memory copies?
8. Как NIC использует DMA?
9. Как RSS масштабирует packet processing?
10. Почему NUMA важен для NIC workloads?

## DPDK

1. Что такое EAL?
2. Что такое PMD?
3. Что такое mbuf?
4. Что такое mempool?
5. Почему DPDK использует huge pages?
6. Как DPDK получает packets без syscall на каждый packet?
7. Как работают RX/TX queues?
8. Что такое burst processing?
9. Как распределить traffic между cores?
10. Как измерить DPDK application performance?

## RDMA

1. Что такое RDMA?
2. Что такое QP?
3. Что такое CQ?
4. Что такое MR?
5. Почему требуется memory registration?
6. RDMA Write vs Send?
7. Что такое RoCE?
8. Почему PFC может быть нужен?
9. Как измерять RDMA latency?
10. Какие trade-offs есть между RDMA и TCP?

---

# Senior-Level System Design Questions

После технических тем нужно уметь отвечать уже не на "что такое", а на "как спроектировать".

## Question 1

**Design a 100 Gbps user-space packet processing service.**

Нужно обсудить:

- packet rate;
- CPU budget;
- RSS;
- queues;
- NUMA;
- DMA;
- batching;
- memory allocation;
- backpressure;
- observability;
- failure handling.

## Question 2

**Design a low-latency TCP proxy.**

Обсудить:

- epoll;
- connection management;
- buffering;
- partial writes;
- `TCP_NODELAY`;
- backpressure;
- CPU affinity;
- memory allocation;
- p99 latency.

## Question 3

**How would you debug a sudden 30% throughput regression?**

Предложенный порядок:

```text
1. Reproduce
2. Define baseline
3. Check workload
4. Check CPU
5. Check memory
6. Check network
7. Profile
8. Compare traces
9. Identify bottleneck
10. Change one thing
11. Benchmark again
```

## Question 4

**How would you design a scalable packet-processing pipeline for many CPU cores?**

Обсудить:

```text
NIC
 |
RSS
 |
RX queues
 |
CPU cores
 |
per-core state
 |
TX queues
```

Главные темы:

- avoiding shared state;
- cache locality;
- NUMA;
- lock contention;
- queue affinity.

---

# Final Interview Checklist

Перед интервью ты должен уверенно объяснять следующие цепочки.

## TCP send path

```text
Application
    |
send()
    |
socket buffer
    |
TCP
    |
IP
    |
routing
    |
qdisc
    |
driver
    |
DMA
    |
NIC
    |
wire
```

## TCP receive path

```text
wire
  |
NIC
  |
DMA
  |
driver
  |
NAPI / kernel networking
  |
IP
  |
TCP
  |
socket receive buffer
  |
recv()
  |
Application
```

## Kernel bypass

```text
Application
   |
userspace networking
   |
DMA buffers
   |
NIC
   |
wire
```

## High-performance packet processing

```text
NIC
 |
RSS
 |
RX queues
 |
CPU cores
 |
per-core processing
 |
TX queues
 |
NIC
```

---

# Recommended Priority for Your Background

С учётом твоего C++ и multithreading background, не трать одинаковое время на все темы.

### 🔴 Максимальный приоритет

1. TCP/IP internals
2. Linux socket API
3. `epoll` / event-driven networking
4. Linux networking path
5. kernel bypass
6. DPDK
7. low-latency performance
8. NIC queues / DMA / RSS

### 🟠 Средний приоритет

9. NUMA / cache
10. RDMA / RoCE
11. Linux kernel / driver basics
12. zero-copy
13. PCIe

### 🟡 После этого

14. ConnectX
15. BlueField
16. DOCA
17. advanced RDMA features

---

# What "Interview Ready" Looks Like

Ты готов, когда можешь без документации:

1. Нарисовать TCP data path в Linux.
2. Объяснить, почему `recv()` не обязан соответствовать `send()`.
3. Объяснить `epoll` ET и `EAGAIN`.
4. Объяснить SYN queue / accept queue.
5. Объяснить TIME_WAIT.
6. Объяснить flow control vs congestion control.
7. Объяснить kernel bypass.
8. Объяснить DMA и descriptor rings.
9. Объяснить DPDK architecture.
10. Объяснить RSS и multi-queue NIC.
11. Объяснить NUMA impact.
12. Спроектировать low-latency TCP/UDP service.
13. Предложить методику поиска performance bottleneck.
14. Интерпретировать `tcpdump` и базовый `perf`.
15. Объяснить, почему конкретная optimization улучшает p99/throughput.

# Suggested Study Sequence

```text
Stage 0   Linux + sockets
   ↓
Stage 1   TCP/IP
   ↓
Stage 2   TCP internals
   ↓
Stage 3   epoll / event loop
   ↓
Stage 4   Linux networking internals
   ↓
Stage 5   zero-copy / memory
   ↓
Stage 6   CPU / NUMA / low latency
   ↓
Stage 7   kernel bypass
   ↓
Stage 8   userspace UDP stack
   ↓
Stage 9   DPDK
   ↓
Stage 10  RDMA
   ↓
Stage 11  ConnectX / BlueField
   ↓
Stage 12  Linux kernel / driver
   ↓
Stage 13  performance investigation
   ↓
Stage 14  final project
```

Главный принцип: **не пытайся выучить весь networking stack энциклопедически**. Для Senior-позиции важно уметь связать уровни:

```text
C++
  ↓
Linux
  ↓
syscalls
  ↓
TCP/IP
  ↓
driver
  ↓
DMA / PCIe
  ↓
NIC
  ↓
performance
```

И уметь переходить между ними, когда возникает проблема.
