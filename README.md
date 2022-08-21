# cpu-scheduling-simulator
A simulator written in C for the purposes of learning and solving CPU scheduling algorithms like Shortest Remaining Time First, Priority, Round Robin, etc.

It makes use of queue.h for the process queue (see https://man.openbsd.org/queue.3)
Inputs can either be randomly generated or inserted in manually
The output contains a gantt chart representing the scheduling and other calculations

Help menu when no arguments are passed:

![gh1](https://user-images.githubusercontent.com/83671362/185805956-00234708-3b4f-4a1f-af0e-dcf040c1fc0a.png)

Example for Shortest Job First (random first):

![gh2](https://user-images.githubusercontent.com/83671362/185805995-eaf57a1e-b348-49bf-84b9-3ec47ccdc9f1.png)
