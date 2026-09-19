# Latență și jitter

## 1. Problema

Un task periodic ar trebui să pornească exact la momentele stabilite: la 0, 10,
20, 30 ms și așa mai departe. În realitate pornește puțin mai târziu, și nu cu
aceeași întârziere de fiecare dată. Pentru un regulator numeric, o eșantionare
neregulată înseamnă o comandă calculată pe date „vechi” cu o vârstă
necunoscută, iar bucla de reglare își pierde din stabilitate. Pentru un sistem
audio înseamnă zgomot. Trebuie deci să putem spune *cât* de neregulat este un
sistem și *de unde* vine neregularitatea.

## 2. Ideea

Se compară momentele reale de pornire cu o grilă ideală:

```
grila ideală    |         |         |         |         |
pornire reală   |>        |--->     |>        |------>  |->
întârziere      mică      medie     mică      mare      mică
```

Două mărimi descriu comportarea: cât de mare este întârzierea în cel mai rău
caz și cât de mult variază ea de la o perioadă la alta. Prima hotărăște dacă
termenele sunt respectate; a doua hotărăște calitatea unui semnal eșantionat
sau a unei bucle de reglare.

Ideea centrală a paginii: într-un sistem de timp real contează **maximul** și
forma **distribuției**, nu media. O medie foarte bună poate ascunde un caz rar
care strică tot.

## 3. Teorie

**Latența** (latency) este timpul scurs de la un eveniment până la reacția la
el: de la expirarea unui temporizator până la pornirea taskului, de la un front
pe un pin până la prima instrucțiune a rutinei de întrerupere.

**Întârzierea** (lateness) a perioadei $k$ este diferența dintre momentul real
al pornirii și momentul ideal de pe grilă:

$$
L_k = t_k - (t_0 + k \cdot T).
$$

**Jitterul** (jitter) este variația întârzierii:

$$
J = \max_k L_k - \min_k L_k.
$$

Un sistem cu întârziere mare, dar constantă, are jitter zero: este doar decalat
în timp, iar decalajul se poate compensa. Un sistem cu întârziere variabilă nu
se poate compensa, pentru că variația nu este cunoscută dinainte.

**De ce maximul.** Un termen limită este ratat de *un* job, nu de media
joburilor. Dacă 9 999 de perioade au întârzierea de 20 µs și una are 3 ms,
media este practic 20 µs, dar sistemul a avut o abatere de 3 ms. Garanțiile de
timp real se dau pentru cel mai rău caz.

**Histograma.** Media, minimul și maximul nu arată cât de des apar valorile
mari. O histogramă a întârzierilor arată forma distribuției: un vârf îngust
lângă zero este comportarea obișnuită, iar **coada** (tail) din dreapta conține
cazurile rare. Un maxim măsurat este doar o limită inferioară a maximului
adevărat: o măsurătoare mai lungă poate găsi o valoare și mai mare. De aceea
măsurarea se completează cu analiză (vezi
[Analiza timpului de răspuns](../3-analysis/response-time-analysis.md)).

**Sursele jitterului**, de la cele mai mici la cele mai mari:

| Sursa | Mecanismul | Se poate elimina ridicând prioritatea? |
|---|---|---|
| granularitatea tickului | trezirea are loc doar la un tick, iar tratarea tickului durează ea însăși puțin și variabil | nu |
| secțiuni critice | cât întreruperile sunt oprite, nici tickul nu este tratat | nu |
| întreruperi | orice rutină de întrerupere rulează înaintea oricărui task | nu |
| taskuri ale sistemului | activitatea radio sau de rețea rulează, de obicei, deasupra tuturor taskurilor aplicației | nu |
| taskuri mai prioritare ale aplicației | taskul așteaptă până când acestea își termină calculul | da |

Doar ultima sursă ține de alegerea priorităților. Celelalte sunt sub nivelul la
care acționează planificatorul de taskuri, deci nicio prioritate de task nu le
poate ocoli. Remediile sunt de altă natură: secțiuni critice și rutine de
întrerupere scurte (vezi
[Secțiuni critice](../4-interrupts/critical-sections.md)), mutarea activității
perturbatoare pe alt nucleu sau pe alt procesor, ori generarea semnalului
critic direct în hardware, cu un temporizator.

Ridicarea priorității unui task nu face jitterul să dispară din sistem, ci îl
mută: taskurile rămase dedesubt vor fi ele întârziate de taskul ridicat.

## 4. Exemplu

Un task periodic cu $T = 10$ ms are deasupra lui un task mai prioritar care
calculează 2 ms la fiecare 9 ms.

- Dacă taskul mai prioritar tocmai a pornit când trebuie să pornească taskul
  nostru, acesta așteaptă toate cele 2 ms. Dacă taskul mai prioritar este la
  jumătatea calculului, așteaptă 1 ms. Dacă doarme, nu așteaptă deloc.
- Perioadele de 9 și 10 ms nu sunt multiple una alteia, deci poziția relativă a
  celor două taskuri alunecă cu 1 ms la fiecare perioadă și se repetă după
  90 ms. Pe o grilă de 1 ms, întârzierea ia valorile 0, 1 și 2 ms.
- Rezultă: întârziere maximă 2 ms, întârziere minimă 0, jitter $J = 2$ ms.
  Întârzierea medie este mult mai mică, pentru că în cele mai multe perioade
  taskul mai prioritar doarme.

Dacă ridicăm taskul nostru deasupra celuilalt, jitterul lui scade la nivelul
dat de tick și de întreruperi, iar cel care suferă este fostul task prioritar:
el va aștepta acum până la durata de calcul a taskului nostru.

## 5. Greșeli frecvente

- **Judecarea după medie.** „Întârzierea medie este de 15 µs” nu spune nimic
  despre respectarea unui termen. Se raportează maximul și histograma.
- **„Prioritatea maximă înseamnă jitter zero.”** Întreruperile, secțiunile
  critice și taskurile sistemului rămân deasupra oricărui task al aplicației.
- **„Maximul măsurat este cazul cel mai rău.”** Este cel mai rău caz *văzut*.
  Cazul cel mai rău adevărat poate cere o potrivire de momente care nu a apărut
  în timpul măsurătorii.

## 6. Întrebări de verificare

1. Un sistem are întârzierea între 480 și 500 µs, altul între 5 și 300 µs. Care
   are latența mai bună și care are jitterul mai bun? Care este de preferat
   pentru o buclă de reglare și de ce?
2. De ce nu poate fi eliminat prin alegerea priorităților jitterul provocat de
   o rutină de întrerupere?
3. O măsurătoare de 10 minute arată o întârziere maximă de 120 µs. Ce puteți
   afirma despre cazul cel mai rău al sistemului și ce nu puteți afirma?
4. În exemplul din secțiunea 4, ce s-ar schimba dacă taskul mai prioritar ar
   avea perioada de exact 10 ms?

Pentru mai mult: G. Buttazzo, *Hard Real-Time Computing Systems*, cap. 1 și 4;
H. Kopetz, *Real-Time Systems*, cap. 3 și 9; A. Cervin și colab., „How Does
Control Timing Affect Performance?”, *IEEE Control Systems Magazine*, 2003.

Laborator asociat: [Lab 1](../../lab1-time-latency/README.md).
