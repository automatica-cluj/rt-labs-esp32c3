# Bibliografie

Paginile de concepte sunt scurte intenționat. Teoria completă, cu
demonstrații, se găsește în lucrările de mai jos.

## Cărți

- G. C. Buttazzo, *Hard Real-Time Computing Systems: Predictable Scheduling
  Algorithms and Applications*, ed. a 3-a, Springer, 2011. Cartea de referință
  pentru aproape toate paginile: cap. 1 (noțiuni de bază), cap. 4 (planificarea
  taskurilor periodice, RM, analiza timpului de răspuns), cap. 7 (protocoale
  de acces la resurse), cap. 9 (supraîncărcare).
- A. Burns, A. Wellings, *Real-Time Systems and Programming Languages*, ed. a
  4-a, Addison-Wesley, 2009. Planificare, resurse partajate și toleranță la
  defecte, cu exemple de cod.
- R. Barry, *Mastering the FreeRTOS Real Time Kernel*, disponibilă gratuit pe
  freertos.org. Taskuri, cozi, semafoare, mutexuri, întreruperi, cu exemple.

## Articole clasice

- C. L. Liu, J. W. Layland, „Scheduling Algorithms for Multiprogramming in a
  Hard-Real-Time Environment”, *Journal of the ACM*, 20(1), 1973. Rate
  Monotonic și limita de utilizare (Lab 3).
- M. Joseph, P. Pandya, „Finding Response Times in a Real-Time System”, *The
  Computer Journal*, 29(5), 1986. Analiza timpului de răspuns (Lab 3).
- N. Audsley, A. Burns, M. Richardson, K. Tindell, A. Wellings, „Applying New
  Scheduling Theory to Static Priority Pre-emptive Scheduling”, *Software
  Engineering Journal*, 8(5), 1993. Analiza timpului de răspuns cu blocare și
  jitter.
- L. Sha, R. Rajkumar, J. P. Lehoczky, „Priority Inheritance Protocols: An
  Approach to Real-Time Synchronization”, *IEEE Transactions on Computers*,
  39(9), 1990. Moștenirea și plafonul de prioritate (Lab 5).
- E. G. Coffman, M. Elphick, A. Shoshani, „System Deadlocks”, *ACM Computing
  Surveys*, 3(2), 1971. Cele patru condiții ale deadlock-ului (Lab 5).
- R. Wilhelm și colab., „The Worst-Case Execution-Time Problem — Overview of
  Methods and Survey of Tools”, *ACM Transactions on Embedded Computing
  Systems*, 7(3), 2008. WCET măsurat și analizat (Lab 3).

## Studii de caz

- G. Reeves, „What really happened on Mars”, 1997. Relatarea echipei Mars
  Pathfinder despre inversiunea de prioritate de pe sondă (Lab 5).

## Partea practică

Documentația sistemului de operare și a plăcii folosite în laborator, precum și
paginile despre programare, se găsesc în lucrările de laborator și în dosarul
`docs/` al depozitului, nu aici.
