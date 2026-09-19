# Excludere mutuală

Citiți înainte: [Condiții de cursă](race-conditions.md).

---

## 1. Problema

O dată partajată, modificată în mai mulți pași de două taskuri, își pierde
coerența. Remediul este **excluderea mutuală** (mutual exclusion): cât timp un
task se află în secțiunea critică, niciun altul nu poate intra în ea.

Există mai multe mecanisme, iar într-un sistem de timp real alegerea contează,
pentru că fiecare mecanism întârzie *pe altcineva*. Întrebarea nu este doar
„este corect?”, ci și „cine așteaptă, și cât?”.

## 2. Ideea

Trei feluri de a proteja aceeași secțiune critică:

| Mecanism | Cum protejează | Cine este întârziat |
|---|---|---|
| **mutex** | taskul care găsește zăvorul ocupat se blochează până la eliberare | doar taskurile care vor *aceeași* resursă |
| **oprirea întreruperilor** | fără întreruperi nu există tick, deci nici comutare de task | *tot* sistemul: toate taskurile și toate întreruperile |
| **operație atomică** | modificarea devine un singur pas indivizibil | practic nimeni; valabil doar pentru o singură variabilă |

```
mutex:                  T1 [ia]====secțiune critică====[dă]
                        T2 ......cere, se blochează.........[ia]====[dă]
                        T3 (altă resursă) rulează nestingherit după prioritate

oprirea întreruperilor: T1 [oprește]==secțiune critică==[pornește]
                        orice altceva, inclusiv întreruperile, așteaptă
```

Al patrulea fel este să *nu împărțim* datele: un task trimite celuilalt o
copie printr-o **coadă** (queue). Nu mai există date comune, deci nici
secțiune critică în codul aplicației.

## 3. Teorie

**Mutex**: zăvor cu *proprietar*. Doar taskul care l-a luat îl poate elibera,
iar nucleul știe cine îl ține, deci poate aplica moștenirea priorității. Costă
două apeluri de nucleu la fiecare intrare și, când este ocupat, o blocare și o
comutare de context. Nu poate fi folosit dintr-o rutină de întrerupere, pentru
că aceasta nu se poate bloca.

**Semafor binar**: are aceleași două stări, dar *nu are proprietar*. Este un
mecanism de **semnalizare** („s-a întâmplat ceva”), nu de protecție. Folosit ca
zăvor, nu oferă moștenirea priorității și deschide calea
[inversiunii de prioritate](priority-inversion.md) nemărginite.

**Secțiune critică prin oprirea întreruperilor**: foarte ieftină și singura
care protejează datele împărțite cu o rutină de întrerupere. Durata ei se
adaugă direct la latența tuturor întreruperilor (vezi
[Secțiuni critice](../4-interrupts/critical-sections.md)).

**Operație atomică**: suport hardware sau, în lipsa lui, o secțiune critică de
câteva instrucțiuni generată de compilator.

Regula de alegere, după *cât durează* secțiunea critică și *cine* o folosește:

| Situația | Mecanismul |
|---|---|
| un contor sau un fanion | operație atomică |
| câteva instrucțiuni; date împărțite cu o rutină de întrerupere | oprirea întreruperilor |
| o resursă ținută mai mult timp (un buffer, un periferic, o magistrală) | mutex |
| date care trec de la un task la altul | coadă |

Reguli valabile pentru toate:

- Secțiunea critică trebuie să fie **cât mai scurtă**: în ea se copiază datele,
  prelucrarea se face în afara ei. Durata ei este exact timpul cu care poate fi
  întârziat un task mai prioritar, adică termenul de blocare $B_i$ din analiza
  timpului de răspuns.
- **Fără așteptări** în secțiunea critică: durata ei trebuie să fie cunoscută.
- Toți cei care folosesc data, **inclusiv cititorii**, trec prin același
  mecanism.

## 4. Exemplu

Un task de control cu perioada de 1 ms și un task de jurnalizare împart un
buffer. Jurnalizarea are nevoie de buffer 2 ms pentru a-l prelucra.

| Varianta | Efectul asupra taskului de control |
|---|---|
| întreruperi oprite 2 ms | nicio întrerupere și niciun task timp de 2 ms: controlul pierde sigur o perioadă |
| mutex ținut 2 ms | controlul este blocat până la 2 ms doar dacă cere bufferul în acel interval; altfel rulează normal |
| mutex ținut doar cât se copiază bufferul (de exemplu 20 µs), prelucrare pe copie | blocare de cel mult 20 µs |

A treia variantă este cea corectă: se scurtează secțiunea critică, nu se caută
un mecanism mai „puternic”.

## 5. Greșeli frecvente

- **Secțiune critică lungă „ca să fie sigur”.** Tot ce este în ea se adaugă la
  întârzierea altora.
- **Semafor binar folosit ca mutex.** Funcționează la test și produce
  inversiune de prioritate nemărginită în exploatare.
- **Protecție doar la scriere.** Și cititorul unei structuri cu mai multe
  câmpuri poate vedea o stare pe jumătate actualizată.

## 6. Întrebări de verificare

1. De ce este mutexul mai scump decât oprirea întreruperilor, și de ce rămâne
   totuși alegerea corectă pentru o resursă ținută milisecunde?
2. Un contor este modificat de o rutină de întrerupere și de un task. Ce
   mecanisme se pot folosi și care nu? De ce?
3. În loc să împartă o variabilă, producătorul trimite valorile printr-o
   coadă. Ce problemă dispare și ce cost apare în schimb?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 7;
R. Barry, *Mastering the FreeRTOS Real Time Kernel*, cap. despre gestiunea
resurselor.

Laborator asociat: [Lab 5](../../lab5-shared-resources/README.md).
