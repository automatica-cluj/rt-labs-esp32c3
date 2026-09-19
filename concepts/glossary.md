# Glosar

Termenii folosiți în paginile de concepte, cu echivalentul englezesc din
literatură și pagina pe care sunt explicați.

| Termen | Engleză | Pe scurt | Pagina |
|---|---|---|---|
| acuratețe | accuracy | cât de aproape este valoarea citită de timpul adevărat | [Ceasuri](1-time/clocks-and-resolution.md) |
| așteptare activă | busy waiting | așteptare în care taskul ține procesorul | [Ceasuri](1-time/clocks-and-resolution.md) |
| așteptare circulară | circular wait | fiecare task dintr-un ciclu așteaptă o resursă ținută de următorul | [Deadlock](5-shared-resources/deadlock.md) |
| atomică (operație) | atomic | se execută complet sau deloc, fără stări intermediare vizibile | [Condiții de cursă](5-shared-resources/race-conditions.md) |
| blocare | blocking | întârziere provocată de un task cu prioritate *mai mică* | [Inversiunea de prioritate](5-shared-resources/priority-inversion.md) |
| cedare voluntară | yield | taskul renunță singur la procesor | [Înfometare și watchdog](2-scheduling/starvation-and-watchdogs.md) |
| coadă | queue | transmite date între taskuri prin copiere | [Excludere mutuală](5-shared-resources/mutual-exclusion.md) |
| coadă de taskuri gata | ready queue | taskurile care ar putea rula, ordonate după prioritate | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| comutare de context | context switch | salvarea unui task și reluarea altuia | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| condiție de cursă | race condition | rezultatul depinde de momentul comutării între taskuri | [Condiții de cursă](5-shared-resources/race-conditions.md) |
| contor de cicluri | cycle counter | numără ciclurile de ceas ale procesorului; cea mai fină sursă de timp | [Ceasuri](1-time/clocks-and-resolution.md) |
| deadlock (interblocare) | deadlock | taskuri care se așteaptă reciproc la nesfârșit | [Deadlock](5-shared-resources/deadlock.md) |
| degradare controlată | graceful degradation | la supraîncărcare se renunță întâi la ce este mai puțin important | [Supraîncărcare](3-analysis/overload.md) |
| depășirea perioadei | overrun | un job care durează mai mult decât perioada lui | [Eliberare periodică](1-time/periodic-release.md) |
| derivă | drift | întârziere care se adună de la o perioadă la alta | [Eliberare periodică](1-time/periodic-release.md) |
| efect de sondă | probe effect | măsurarea modifică ceea ce se măsoară | [Ceasuri](1-time/clocks-and-resolution.md) |
| eliberare | release | momentul în care un job devine gata de execuție | [Modelul de task](3-analysis/task-model.md) |
| excludere mutuală | mutual exclusion | un singur task în secțiunea critică la un moment dat | [Excludere mutuală](5-shared-resources/mutual-exclusion.md) |
| factor de utilizare ($U$) | utilisation | suma $C_i/T_i$: fracțiunea de procesor cerută | [Modelul de task](3-analysis/task-model.md) |
| felii de timp | time slicing | rotirea la fiecare tick a taskurilor cu aceeași prioritate | [Preempțiune](2-scheduling/fixed-priority-preemption.md) |
| filtrarea vibrațiilor | debouncing | a obține o singură apăsare din multele fronturi ale unui contact | [Debouncing](4-interrupts/debouncing.md) |
| front | edge | trecerea unui semnal din LOW în HIGH sau invers | [Debouncing](4-interrupts/debouncing.md) |
| graf de așteptare | wait-for graph | cine așteaptă după cine; un ciclu înseamnă deadlock | [Deadlock](5-shared-resources/deadlock.md) |
| grilă de eliberări | release grid | momentele ideale $t_0 + kT$ ale eliberărilor | [Eliberare periodică](1-time/periodic-release.md) |
| hiperperioadă | hyperperiod | cel mai mic multiplu comun al perioadelor | [Modelul de task](3-analysis/task-model.md) |
| histogramă | histogram | câte eșantioane cad în fiecare interval de valori | [Latență și jitter](1-time/latency-and-jitter.md) |
| înfometare | starvation | un task gata de execuție care nu mai primește procesorul | [Înfometare și watchdog](2-scheduling/starvation-and-watchdogs.md) |
| instant critic | critical instant | toate taskurile eliberate simultan: cel mai rău caz | [Modelul de task](3-analysis/task-model.md) |
| întârziere | lateness | trezirea reală minus momentul ideal al eliberării | [Latență și jitter](1-time/latency-and-jitter.md) |
| întârziere relativă / absolută (a execuției) | relative / absolute delay | „așteaptă T de acum” față de „așteaptă până la eliberarea precedentă + T” | [Eliberare periodică](1-time/periodic-release.md) |
| interferență | interference | întârziere provocată de taskuri cu prioritate *mai mare* | [Preempțiune](2-scheduling/fixed-priority-preemption.md) |
| întrerupere | interrupt | cerere hardware care oprește pe loc taskul curent | [Întreruperi și latență](4-interrupts/interrupts-and-latency.md) |
| inversiune de prioritate | priority inversion | un task prioritar așteaptă după unul mai puțin prioritar | [Inversiunea de prioritate](5-shared-resources/priority-inversion.md) |
| ISR (rutină de tratare a întreruperii) | interrupt service routine | funcția executată la o întrerupere | [Întreruperi și latență](4-interrupts/interrupts-and-latency.md) |
| iterație de punct fix | fixed-point iteration | se recalculează $R$ până când nu se mai schimbă | [Analiza timpului de răspuns](3-analysis/response-time-analysis.md) |
| jitter | jitter | întârzierea maximă minus cea minimă | [Latență și jitter](1-time/latency-and-jitter.md) |
| job | job | o execuție a unui task periodic | [Modelul de task](3-analysis/task-model.md) |
| latență | latency | timpul de la un eveniment până la reacția la el | [Latență și jitter](1-time/latency-and-jitter.md) |
| latența întreruperii | interrupt latency | de la evenimentul hardware până la prima linie din ISR | [Întreruperi și latență](4-interrupts/interrupts-and-latency.md) |
| latența ISR–task | ISR-to-task latency | de la ISR până când rulează taskul trezit de el | [Procesare amânată](4-interrupts/deferred-processing.md) |
| limita hiperbolică | hyperbolic bound | test suficient mai puțin pesimist: $\prod (U_i + 1) \le 2$ | [Rate Monotonic](3-analysis/rate-monotonic.md) |
| limita Liu & Layland | Liu & Layland bound | $U \le n(2^{1/n} - 1)$: test suficient pentru RM | [Rate Monotonic](3-analysis/rate-monotonic.md) |
| livelock | livelock | taskurile rulează, dar nu avansează | [Deadlock](5-shared-resources/deadlock.md) |
| margine de siguranță | safety margin | adaos peste maximul măsurat, pentru cazurile neobservate | [WCET](3-analysis/wcet.md) |
| moștenirea priorității | priority inheritance | cel care ține zăvorul rulează cu prioritatea celui care îl așteaptă | [Inversiunea de prioritate](5-shared-resources/priority-inversion.md) |
| mutex | mutex | zăvor cu proprietar și moștenirea priorității | [Excludere mutuală](5-shared-resources/mutual-exclusion.md) |
| notificare | task notification | trezirea directă a unui anumit task, de obicei dintr-un ISR | [Procesare amânată](4-interrupts/deferred-processing.md) |
| perioadă ($T$) | period | intervalul dintre două eliberări | [Modelul de task](3-analysis/task-model.md) |
| perioade armonice | harmonic periods | fiecare perioadă este multiplu al celor mai scurte | [Rate Monotonic](3-analysis/rate-monotonic.md) |
| plafon de prioritate | priority ceiling | prioritatea celui mai prioritar task care folosește un zăvor | [Inversiunea de prioritate](5-shared-resources/priority-inversion.md) |
| planificabil | schedulable | toate termenele sunt respectate în cel mai rău caz | [Rate Monotonic](3-analysis/rate-monotonic.md) |
| planificator | scheduler | partea din nucleu care alege taskul care rulează | [Preempțiune](2-scheduling/fixed-priority-preemption.md) |
| politică de depășire: omitere / recuperare | overrun policy: skip / catch-up | ce se întâmplă cu joburile care au întârziat | [Supraîncărcare](3-analysis/overload.md) |
| preempțiune | preemption | oprirea taskului curent în favoarea unuia mai prioritar | [Preempțiune](2-scheduling/fixed-priority-preemption.md) |
| procesare amânată | deferred processing | ISR scurt care trezește un task; munca se face în task | [Procesare amânată](4-interrupts/deferred-processing.md) |
| Rate Monotonic (RM) | rate monotonic | perioadă mai scurtă, prioritate mai mare | [Rate Monotonic](3-analysis/rate-monotonic.md) |
| rezoluție | resolution | cel mai mic pas pe care îl poate arăta un ceas | [Ceasuri](1-time/clocks-and-resolution.md) |
| secțiune critică | critical section | cod care nu are voie să fie intercalat cu alt cod ce folosește aceleași date; se obține cu un zăvor sau prin oprirea întreruperilor | [Secțiuni critice](4-interrupts/critical-sections.md) |
| semafor binar | binary semaphore | semnal „s-a întâmplat”; fără proprietar, fără moștenire | [Inversiunea de prioritate](5-shared-resources/priority-inversion.md) |
| stare (Running, Ready, Blocked, Suspended) | task state | în execuție, gata, blocat, suspendat | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| stivă | stack | memoria proprie a unui task | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| supraîncărcare | overload | $U > 1$: nu toate termenele pot fi respectate | [Supraîncărcare](3-analysis/overload.md) |
| supraîncărcare permanentă / tranzitorie | permanent / transient overload | cererea depășește mereu, respectiv doar temporar, procesorul | [Supraîncărcare](3-analysis/overload.md) |
| task | task | funcție cu stivă și prioritate proprii, oprită și reluată de planificator | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| taskul idle | idle task | rulează când niciun alt task nu este gata | [Taskuri și stări](2-scheduling/tasks-and-states.md) |
| termen limită ($D$) | deadline | momentul până la care jobul trebuie să termine | [Modelul de task](3-analysis/task-model.md) |
| tick | tick | întreruperea periodică de 1 ms a planificatorului | [Ceasuri](1-time/clocks-and-resolution.md) |
| timp de calcul ($C$) | computation time | timpul de procesor cerut de un job | [Modelul de task](3-analysis/task-model.md) |
| timp de răspuns ($R$) | response time | de la eliberare până la terminarea jobului | [Analiza timpului de răspuns](3-analysis/response-time-analysis.md) |
| timp real (hard, firm, soft) | real time | un rezultat întârziat este un rezultat greșit | [Ce înseamnă timp real](00-what-is-real-time.md) |
| vibrația contactelor | contact bounce | închideri și deschideri repetate ale unui contact mecanic | [Debouncing](4-interrupts/debouncing.md) |
| watchdog | watchdog | temporizator care resetează cipul dacă nu este „hrănit” la timp | [Înfometare și watchdog](2-scheduling/starvation-and-watchdogs.md) |
| WCET | worst-case execution time | timpul de execuție în cazul cel mai defavorabil | [WCET](3-analysis/wcet.md) |
| zăvor | lock | orice obiect care asigură excluderea mutuală | [Excludere mutuală](5-shared-resources/mutual-exclusion.md) |
