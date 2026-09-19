# Filtrarea vibrațiilor contactelor (debouncing)

Citiți înainte: [Procesare amânată](deferred-processing.md).

## 1. Problema

Un buton, un releu sau un limitator de cursă este un contact mecanic. La
închidere, lamelele lui se lovesc, sar înapoi și se ating din nou de mai multe
ori înainte să se așeze. Electric, o singură apăsare arată ca un tren de
impulsuri: aceasta este **vibrația contactelor** (contact bounce).

Pentru un om, vibrația este invizibilă, pentru că durează mult sub o zecime de
secundă. Pentru un sistem care reacționează în microsecunde, fiecare impuls
este un eveniment separat. Un contor de piese ar număra aceeași piesă de mai
multe ori; o comandă pornit/oprit ar comuta de câteva ori la o singură apăsare
și ar rămâne într-o stare întâmplătoare.

## 2. Ideea

Semnalul unui contact la o apăsare și o eliberare:

```
nivel
 sus   ────┐ ┌┐ ┌─┐                        ┌┐ ┌──────────
           │ ││ │ │                        ││ │
 jos       └─┘└─┘ └────────────────────────┘└─┘
           |<-vibrație->|<--- stabil --->|<-vibrație->|
fronturi:  1 2 3 4 5                       6 7 8
```

Opt **fronturi** (edges), dar un singur eveniment de apăsare și unul de
eliberare. **Filtrarea vibrațiilor** (debouncing) înseamnă obținerea
evenimentelor logice din trenul de fronturi.

Ideea de bază: o schimbare este reală doar dacă semnalul a rămas **liniștit**
un timp mai lung decât cea mai lungă vibrație posibilă. După fiecare front se
așteaptă; dacă mai vine un front, așteptarea reîncepe. Când perioada de liniște
s-a scurs fără niciun front, nivelul se citește o singură dată și se compară cu
ultima stare stabilă cunoscută.

## 3. Teorie

**Front** (edge): trecerea semnalului de la un nivel la celălalt.

**Perioadă de liniște** (settle time) $t_s$: intervalul fără fronturi cerut
înainte ca nivelul să fie considerat stabil.

Alegerea lui $t_s$ este un compromis între două limite:

$$
t_{vibrație,\,max} \;<\; t_s \;<\; t_{eveniment,\,min}
$$

- dacă $t_s$ este mai mic decât cea mai lungă pauză dintre două fronturi ale
  aceleiași vibrații, filtrul lasă să treacă evenimente false;
- dacă $t_s$ este mai mare decât cea mai scurtă apăsare reală sau decât pauza
  dintre două apăsări reale, filtrul pierde sau unește evenimente adevărate.

Pentru contacte obișnuite vibrația durează de la sute de microsecunde la câteva
milisecunde, iar o apăsare umană cel puțin câteva zeci de milisecunde, deci
există o fereastră largă; o valoare uzuală este de 10–20 ms.

**Latența filtrului.** Evenimentul este raportat abia la $t_s$ după *ultimul*
front, deci filtrarea adaugă o întârziere de cel puțin $t_s$. Pentru o
interfață cu utilizatorul este neglijabilă. Pentru un limitator de cursă care
trebuie să oprească un motor este inacceptabilă, și acolo se reacționează la
*primul* front, iar filtrarea servește doar la ignorarea celor următoare.

**Unde se face așteptarea.** Filtrarea este un caz tipic de
[procesare amânată](deferred-processing.md). ISR-ul doar semnalează că a apărut
un front; așteptarea perioadei de liniște și decizia aparțin unui task. O
așteptare de milisecunde într-un ISR ar bloca toate întreruperile de nivel egal
sau mai mic pe toată durata ei, adică de mii de ori mai mult decât este
acceptabil pentru un ISR.

**Variante.**

| Metoda | Principiul | Observații |
|---|---|---|
| așteptarea liniștii după fronturi | întrerupere pe front, apoi $t_s$ fără fronturi | reacționează doar la schimbări; nu consumă procesor în repaus |
| eșantionare periodică | nivelul se citește la intervale fixe; starea se schimbă după $n$ citiri identice la rând | fără întreruperi, perfect previzibilă; consumă procesor și în repaus |
| ignorarea fronturilor apropiate | primul front contează, următoarele sunt ignorate un timp | reacție imediată; un impuls parazit scurt este luat drept apăsare |
| filtrare hardware | un filtru RC urmat de un comparator cu histerezis netezește semnalul înainte de intrare | fără cost software; componente în plus |

Vibrația apare și la eliberare, nu doar la apăsare, deci filtrul trebuie să
trateze ambele sensuri.

## 4. Exemplu

Un contact vibrează cel mult 2 ms (de exemplu), iar cea mai scurtă apăsare
reală durează 50 ms. Orice $t_s$ între 2 ms și 50 ms este corect; se alege
$t_s = 20$ ms, cu rezervă în ambele părți.

O apăsare produce fronturi la 0, 0,3, 0,9 și 1,4 ms.

| Moment (ms) | Ce se întâmplă |
|---|---|
| 0 | primul front; începe așteptarea de 20 ms |
| 0,3; 0,9; 1,4 | alte fronturi; așteptarea reîncepe de fiecare dată |
| 21,4 | 20 ms de liniște; nivelul este citit o dată: apăsat, diferit de starea stabilă anterioară, deci **o** apăsare |

Evenimentul este raportat la 21,4 ms după primul contact. Fără filtrare ar fi
fost raportate patru evenimente.

Cu $t_s = 1$ ms: între fronturile de la 0,3 și 0,9 ms pauza este de 0,6 ms,
deci filtrul rezistă; dar dacă o vibrație are o pauză de 1,2 ms, filtrul
raportează o apăsare, apoi o eliberare și încă o apăsare, toate false.

Cu eșantionare la 5 ms și $n = 4$ citiri identice: aceeași latență, de
aproximativ 20 ms, și un cost de 200 de citiri pe secundă, indiferent dacă
butonul este atins sau nu.

## 5. Greșeli frecvente

- **„Un front înseamnă o apăsare.”** Un contact mecanic produce mai multe
  fronturi pentru fiecare schimbare reală.
- **„Aștept în ISR până se liniștește.”** Milisecunde petrecute într-un ISR
  înseamnă milisecunde de latență pentru tot sistemul.
- **„Filtrarea este gratuită.”** Adaugă cel puțin $t_s$ la timpul de reacție,
  lucru care trebuie pus în bugetul de timp al funcției respective.

## 6. Întrebări de verificare

1. Ce vede utilizatorul unei comenzi pornit/oprit dacă fiecare front este luat
   drept apăsare și vibrația are un număr par de fronturi? Dar impar?
2. Un contact vibrează cel mult 5 ms, iar evenimentele reale pot fi la 8 ms
   unul de altul. Ce valori poate lua $t_s$? Ce faceți dacă fereastra este prea
   îngustă?
3. De ce are nevoie de filtrare și eliberarea butonului?
4. Pentru un buton de oprire de urgență, care dintre metodele din tabel este
   potrivită și de ce?

Pentru mai mult: J. Ganssle, „A Guide to Debouncing”, The Ganssle Group;
P. Horowitz, W. Hill, *The Art of Electronics*, capitolul despre interfațarea
logicii cu lumea reală; vezi și [Bibliografie](../further-reading.md).

Laborator asociat: [Lab 4](../../lab4-interrupts/README.md).
