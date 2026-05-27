# FPSLocker — Ryazhenka / libryazhahand build

> **Ryazhenka ecosystem fork of [masagrator/FPSLocker](https://github.com/masagrator/FPSLocker), based on the [ppkantorski/FPSLocker](https://github.com/ppkantorski/FPSLocker) rework, ported to [`libryazhahand`](https://github.com/dimasick-git/libryazhahand).**

---

## English (TL;DR)

FPSLocker is a Tesla / Ryazhahand overlay that, in combination with SaltyNX, lets you set a custom display refresh rate and FPS in Nintendo Switch retail games.

This fork:
- Builds against `libryazhahand` instead of `libultrahand` / `libtesla`.
- Signs the produced `.ovl` with the `RYZH` marker (instead of `ULTR`).
- Ships GitHub Actions for automatic build of every commit and PR, and automatic GitHub Releases when a `v*` tag is pushed.
- Has a one-click upstream-sync workflow that pulls changes from `masagrator/FPSLocker` and automatically re-applies all Ryazhenka customisations, so the fork never breaks on sync.
- Lives in the Ryazhenka ecosystem alongside `libryazhahand`, `Ryazhahand-Overlay`, and the rest of our Switch homebrew tooling.

Full project documentation below is in Russian.

---

## Русский (полная документация)

**FPSLocker** — это оверлей для Nintendo Switch, который вместе с SaltyNX позволяет задавать собственную частоту обновления дисплея и FPS в обычных играх. Этот репозиторий — порт под экосистему **Ряженка** (`libryazhahand`).

> [!NOTE]
> Инструмент определяет графический API игры и манипулирует FPS. В отдельных случаях требуются патчи под конкретную версию игры для разблокировки более 30 FPS. В оверлее встроена возможность скачивать конфиги, по которым делаются патчи. Репозиторий с конфигами — [`masagrator/FPSLocker-Warehouse`](https://github.com/masagrator/FPSLocker-Warehouse).<br>
> Максимальный поддерживаемый размер YAML — 32 КБ (может быть увеличен в будущих обновлениях).

> [!WARNING]
> НЕ РЕКОМЕНДУЕТСЯ ИСПОЛЬЗОВАТЬ 60 FPS-ЧИТЫ/МОДЫ ОДНОВРЕМЕННО С ЭТИМ ИНСТРУМЕНТОМ. ИЛИ ЧИТ, ИЛИ FPSLOCKER — НЕ ОБА СРАЗУ! ЭТО ПРИВОДИТ К КОНФЛИКТАМ И КРАШАМ.

---

## Чем эта сборка отличается от оригинала

- Сборка под библиотеку **`libryazhahand`** (форк `libultrahand`). В Makefile подключается `libs/libryazhahand/ryazhahand.mk` (с фолбэком на старое имя `ultrahand.mk`).
- Подпись результирующего `.ovl` — **`RYZH`** вместо `ULTR`. Это нужно, чтобы наш форк Ryazhahand-Overlay понимал, что оверлей собран под нашу экосистему.
- Все упоминания `libultrahand` / `Ultrahand-Overlay` в Makefile и документации заменены на `libryazhahand` / `Ryazhahand-Overlay`.
- Подключён `dimasick-git/libryazhahand` как git submodule в `libs/libryazhahand`.
- Версия проекта берётся из единственного источника — файла `.ryazhenka-version`. Скрипт sync-пайплайна сам прописывает её в Makefile.

## GitHub Actions

Под `.github/workflows/` лежат три воркфлоу:

| Файл | Триггеры | Что делает |
|------|----------|------------|
| `build.yml` | каждый коммит, каждый PR, ручной запуск | Собирает `.ovl` в контейнере `devkitpro/devkita64`, дописывает к версии `+ryazh.<shortsha>`, проверяет подпись `RYZH`, заливает артефакт `FPSLocker.ovl` + `BUILD_INFO.txt` (имя/email автора коммита, тема, дата, SHA, ссылка на запуск). После успешной сборки на `main` — авто-обновляет **rolling-релиз `latest-build`** свежим бинарником. |
| `release.yml` | пуш тега `v*` или ручной запуск с указанием тега | Собирает релизный `.ovl`, синхронизирует APP_VERSION с тегом, генерирует release notes и публикует **стабильный** GitHub Release с `.ovl`, `BUILD_INFO.txt` и `.zip`. |
| `sync-upstream.yml` | по расписанию (Пн 04:17 UTC) или ручной запуск | Тянет изменения из `masagrator/FPSLocker`, делает merge с `-X theirs`, откатывает защищённые файлы, прогоняет ряженочные патчи и открывает PR. |

Подробности — `docs/SYNC.md`.

## Требования

- [Atmosphere CFW](https://github.com/Atmosphere-NX/Atmosphere/releases)
- [Форк SaltyNX от masagrator, версия 1.7.4+](https://github.com/masagrator/SaltyNX/releases)
- Tesla-окружение: [Ryazhahand-Overlay](https://github.com/dimasick-git/Ryazhahand-Overlay/releases) (форк Ultrahand-Overlay из экосистемы Ряженка)
- Тулсет для разгона (и не ждите, что в доке игры пойдут с залоченными 60 FPS без серьёзных частот — 1963/998/2133 в большинстве случаев недостаточно)
- [sys-dock](https://github.com/masagrator/sys-dock/releases) — прочитайте его [README](https://github.com/masagrator/sys-dock/blob/main/README.md), чтобы разблокировать 120 Гц в OLED в доке и починить артефактные горизонтальные полосы.

Как всё настроить целиком: [гайд от masagrator](https://gist.github.com/masagrator/65fcbd5ad09243399268d145aaab899b).

## Установка

Доступны два канала релизов:

- **Stable** — `v*.*.*` теги. Стабильные релизы, выпускаются вручную через `release.yml`. Берите их, если нужна гарантированная версия.
- **Rolling** — тег [`latest-build`](../../releases/tag/latest-build). Авто-обновляется CI после **каждого** push'а в `main` (CI-job `auto-release` в `build.yml`). Это всегда самая свежая сборка с тем же `.ovl`, что лежит в артефактах workflow run-а.

Шаги установки:

1. Скачайте `FPSLocker.ovl` из нужного [GitHub Release](../../releases) или из артефактов CI-сборки.
2. Положите файл в `/switch/.overlays/FPSLocker.ovl` на SD-карте.
3. Откройте оверлей через Ryazhahand-Overlay / Tesla menu.

## Использование

Поддерживаемые языки интерфейса: английский, немецкий, французский, русский, бразильский португальский, китайский упрощённый, китайский традиционный.

Оверлей работает в двух режимах:

### Когда игра запущена

Если игра поддерживается SaltyNX и всё установлено правильно — увидите меню, в первой строке которого написано `NX-FPS plugin is running`.

**tl;dr.** Лучший подход для запуска 30 FPS игр на более высокой частоте:

1. Запустите игру, подключите Switch к интернету, в FPSLocker перейдите в `Advanced Settings`, нажмите `Check/download config file`. Если ваша игра и версия совместимы с репозиторием FPSLocker Warehouse, меню обновится и появится пункт `Convert config to patch`. Нажмите на него, перезапустите игру, теперь меняйте FPS Target в FPSLocker.
2. Зайдите в Advanced Settings — если видите «Set/Active/Available buffers: 2/2/3», нажмите `Set buffering`, выберите `Triple (force)`.

**Объяснение опций**:

- `Interval Mode` — используется NVN и EGL API для управления vsync. Значение 2 означает, что каждый кадр держится минимум в 2 раза дольше — на 60 Гц вы получите максимум 30 FPS. Допустимый диапазон 1–4. Не заданное значение (0) трактуется как 1.
- `Custom FPS Target` — лочит игру на заданный FPS. Если в движке свои внутренние локи FPS, без дополнительных патчей разблокировать > 30 FPS не получится.
- `FPS` — сколько кадров отрендерилось за последнюю секунду в текущей игре. Подтверждает, что лок реально работает.
- `Patch file doesn't exist.` — оверлей уверен, что для корректной работы в этой игре нужен патч FPSLocker, но его нет. См. `tl;dr` выше — как получить конфиг и сконвертить в патч (конфиг может в принципе отсутствовать для вашей игры или версии).
- `Increase/Decrease FPS target` — только в портативном режиме. Меняет FPS Target шагом 5. Мин — 15 FPS, макс — 60 FPS.
- `Change FPS target` — только в доке. Открывает таблицу FPS от 15 до 60 (опционально до 120).
- `Disable custom FPS target` — снимает FPS Target. Что должно стоять в `Interval Mode` в этот момент — не угадать, поэтому пользователь сам отвечает за корректное значение interval mode перед снятием FPS Target.
- `Advanced settings` — подменю:
  - Если игра использует NVN:
    - `Window Sync Wait` — опасная опция: выключение может крашить игры, но в некоторых даёт выигрыш — отключает vsync двойной буферизации ценой мелких графических артефактов (список совместимых игр в конце README). Используйте с осторожностью. Не отображается, если двойного буфера нет.
    - `Set Buffering` — если игра использует не двойной буфер, позволяет принудительно понизить буферизацию (но не повысить — например, double нельзя сделать triple). Понижать стоит только для игр со стабильным 30/60 FPS, но плохим framepacing или большим input lag. Если форсировать double в игре с проседаниями — просадки FPS станут жёсткими. Иногда применяется только при старте — игру придётся перезапустить (меню подскажет). <br>Расшифровка `Set/Active/Available Buffers`:
      - Set — сколько буферов задано через `nvnWindowSetNumActiveTextures`. Если игра им не пользуется — 0. Игра может выставить буфер ниже, чем зарезервированное место. Если он ниже Available — можно использовать вариант `(force)`. Без `(force)` сбросится в дефолт.
      - Active — сколько буферов игра реально использует.
      - Available — сколько буферов передано в NVN. По этому числу можно форсить игру использовать их все.
  - Если игра использует Vulkan:
    - `Set Buffering` — переключение между double и triple буфером. Понижать стоит только для игр со стабильным 30/60 FPS, но плохим framepacing/input lag. Применяется только при старте — игру надо перезапустить.
  - `Convert config to patch file` — если есть подходящий конфиг под текущую игру и версию, создаёт патч, который подтянется при следующем запуске. Сохраняется в `SaltySD/plugins/FPSLocker/patches/*titleid_uppercase*/*buildid_uppercase*.bin`.
  - `Delete patch file` — удаляет ранее сделанный патч.
  - `Check/download config file` — проверяет в Warehouse наличие конфига под текущую игру и версию, скачивает и сравнивает с уже лежащим на SD. Если файлы не совпадают — удаляет старый патч и конфиг, пользователь должен вручную сконвертировать новый конфиг в патч. Ошибка 0x312 — пришёл неожиданный файл с github. Другие коды — проблемы с сетью или сервером github.
  - `Halt unfocused game` — некоторые игры не приостанавливаются при выходе в HOME Menu. С этой опцией ядро принудительно усыпит игру, если она не в фокусе.
- `Display settings` — подменю по частоте обновления дисплея:
  - `Increase refresh rate` — только в портативе. Поднять до 60 Гц.
  - `Decrease refresh rate` — только в портативе. Снизить до 40 Гц (для OLED — до 45 Гц).
  - `Change refresh rate` — только в доке. Выбор из списка.
  - `Handheld Display Sync` / `Docked Display Sync` — включено: три опции выше недоступны, частота меняется только во время игры и синхронизируется с FPS Target.
  - `60 HZ in HOME Menu` — если Handheld Display Sync включён, при выходе в HOME Menu SaltyNX будет всегда ставить 60 Гц в портативе.
  - `Retro Remake Mode` — показывается только для Lite c экранами `InnoLux 2J055IA-27A (Rev B1)` или `Retro Remake SUPER5` (первая ревизия). Retro Remake-дисплеям нужен особый подход к смене частоты, а первая версия SUPER5 спуфит ID существующего дисплея — определить автоматически нельзя, поэтому опцию надо включить вручную. Остальные Retro Remake-дисплеи детектируются автоматически.
  - `Docked Settings` — подменю по частоте обновления внешних дисплеев. Недоступно для Lite. Содержит:
    - `myDP link rate` — `HBR` или `HBR2`. В HBR на не-OLED нельзя поднять выше 75 Гц при 1080p; для OLED зависит от количества активных DP-линий в доке (у всех оригинальных доков их 2 — то есть тот же предел в 75 Гц при 1080p). При пределе 75 Гц при 1080p реальный максимум — 60 Гц, если хотите звук. Подробности в конце README.
    - `Config ID` — имя файла-конфига для текущего подключённого дисплея. Файл лежит в `SaltySD/plugins/FPSLocker/ExtDisplays`.
    - `Allowed refresh rates` — проверить и вручную отредактировать список допустимых частот для текущего внешнего дисплея: 40, 45, 50, 55 Гц. По умолчанию включено только 50.
    - `Display underclock wizard` — мастер проходит частоты 40 → 55 Гц, на каждом шаге пользователь должен подтвердить, что изображение есть, нажав требуемую кнопку. Если 15 секунд тишины — переход к следующей. По окончании — экран `Allowed refresh rates`.
    - `Display overclock wizard` — отображается, только если внешний дисплей рапортует ≥ 70 Гц как максимум. Проходит 70 → max (но не выше 120 Гц), 10 секунд на шаг. По окончании — `Allowed refresh rates`.
    - `Frameskip tester` — проверяет, действительно ли дисплей выводит выбранную частоту. Многие дисплеи поддерживают, например, 50 Гц на входе, но всё равно гонят 60 Гц на матрицу. Инструкции внутри. Меню доступно и в портативе.
    - `Additional settings`:
      - `Allow patches to force 60 Hz` — некоторые FPSLocker-патчи форсят 60 Гц, чтобы починить framepacing в 30 FPS катсценах. Игра при этом паузится на 4 секунды. По умолчанию включено. Выключите — будет применяться только лок FPS, без смены частоты и без задержки.
      - `Use lowest refresh rate for unmatched FPS targets` — например, для 60 Гц дисплея и 35 FPS Target подходящей частоты в `Allowed refresh rates` нет. С этой опцией возьмётся минимальная включённая. Без неё — 60 Гц. По умолчанию выключено.
      - `60 HZ in HOME Menu` — если Docked Display Sync включён, при выходе в HOME Menu SaltyNX всегда поставит 60 Гц на этом дисплее.

### Когда игра не запущена

Доступно два подменю:
- `Games list`<br>
  Список установленных игр (макс 32), первый пункт — «All».<br>
  Внутри каждой игры:
  - `Delete settings` — удалить настройки.
  - `Delete patches` — удалить файл, созданный через `Convert config to patch file`.
- `Display settings` — описано выше.
- `Force English language` — если предпочитаете английский, эта опция форсит его в оверлее. Реализовано через самомодификацию исполняемого файла, поэтому после обновления оверлея до новой версии флаг сбрасывается.

## Информация о смене частоты в портативном режиме

OLED-дисплеи Switch требуют гамма-коррекции после смены частоты. Регистры OLED-панели правятся так, чтобы гамма-кривая была как можно ближе к оригинальной, но шаг регистров большой, поэтому небольшая разница в цветах возможна (хуже всего — 60% яркости при 45 Гц).

Из всех репортов только один LCD-экран дал мелкое мерцание в левом нижнем углу при 40 Гц (`InnoLux P062CCA-AZ2`), но у других пользователей с тем же дисплеем (включая меня) этой проблемы не было.

Retro Remake-дисплеям нужно время на адаптацию к новому сигналу, поэтому смена частоты выполняется с задержкой. Слишком короткая задержка приводит к чёрному экрану — лечится сном/перезагрузкой. Если попали — напишите в Issues, увеличу задержку в SaltyNX.

Я ограничил LCD и Retro Remake-дисплеи минимумом в 40 Гц — ниже нет смысла, плюс безопасность. Для OLED Switch минимум 45 Гц, потому что на 40 Гц он ведёт себя некорректно.

LCD можно разогнать до 70 Гц без явных проблем, но я оставил максимум на 60 Гц. Начиная с 75 Гц все пользователи с оригинальными матрицами рапортовали глюки. OLED выше 60 Гц ведёт себя плохо.

Если Display Sync выключен, кастомная частота не восстанавливается после выхода из сна.

Смена частоты влияет на скорость анимаций OS и Tesla-оверлеев — на более низких частотах они становятся медленнее.

Я не несу ответственности за повреждения, вызванные сменой частоты. Каждый раз при входе в `Display settings` вас встретит предупреждение — пользователь несёт всю ответственность сам. Нужно нажать `Accept` для продолжения.

## Информация о смене частоты в доке

Потолок — 120 Гц, как максимум для оригинального дока и не-OLED-моделей.

Многие дисплеи лочатся на 75 Гц при 1080p, потому что что-то мешает связи Switch ↔ док, и тренировка HBR2 в HOS падает, оставляя соединение в HBR. HBR + 2 линии дают предел 180 МГц — чуть выше требуемого для 1080p 75 Гц. Источник проблемы — Switch и/или сам док. В HBR-режиме звук может не передаваться при > 60 Гц @ 1080p. Ручная тренировка HBR2 ресетит сигнал, HOS пытается восстановить, и любая ручная тренировка блокируется.

OLED — особый случай: Nintendo прикрутила программный лимит на PCIE-линии и форсит HBR. sys-dock сысмодуль обходит это.

По тестам, HOS applets ломаются на 100+ Гц. Если игра пытается открыть applet выбора пользователя — игра может крашнуться. Некоторые игры ломаются сами по себе. Пример — «Batman: The Enemy Within» при закрытии выше определённой частоты падает.

## Сборка из исходников

```bash
git clone --recursive https://github.com/dimasick-git/FPSLocker.git
cd FPSLocker
make
```

Требуется devkitPro с пакетами `switch-dev` и `devkitA64` (контейнер `devkitpro/devkita64` уже содержит всё нужное). В CI всё это уже настроено через `.github/workflows/build.yml`.

## Где лежат конфиги, темы, звуки

FPSLocker трогает три разных места на SD-карте — все они уже завязаны на экосистему Ряженка:

| Что | Где | Кто пишет / читает |
|-----|-----|-------------------|
| Глобальные настройки Tesla-окружения, темы, звуки (sound pack), обои (wallpaper.png) | `/config/ryazhahand/` | `libryazhahand` (через `BASE_CONFIG_PATH = /config/ryazhahand/`). НЕ `/config/ultrahand/`. |
| Per-overlay переопределения FPSLocker — `theme.ini`, `wallpaper.rgba`, `lang/<lang>.json` | `/config/fpslocker/` | задаётся в Makefile через `UI_OVERRIDE_PATH := /config/fpslocker/`, читается libryazhahand из `tesla.cpp`. |
| Патчи и настройки самого FPSLocker (`*.dat`, патчи, конфиги внешних дисплеев) | `/SaltySD/plugins/FPSLocker/` | сам FPSLocker через SaltyNX. |

То есть звуки и темы общие со всеми ryazhahand-оверлеями (читаются из `/config/ryazhahand/sounds`, `/config/ryazhahand/.loaded_sounds/*.wav`, `/config/ryazhahand/themes/*.ini`), а FPSLocker может поверх них поставить свой `theme.ini` в `/config/fpslocker/theme.ini`. Если этих файлов нет — оверлей возьмёт глобальную тему из `/config/ryazhahand/`.

Все пути к `/config/ultrahand/...` в нашем форке уже удалены — их обслуживает libryazhahand, и она смотрит только в `/config/ryazhahand/`.

## Структура репозитория

```
.
├── .github/workflows/    # build, release, sync-upstream
├── .ryazhenka-version    # единый источник версии — читается apply_ryazhenka_patches.sh
├── docs/SYNC.md          # как работает upstream-sync
├── include/              # заголовки + языковые файлы (langs/*.hpp)
├── libs/libryazhahand    # git submodule — наша библиотека UI
├── scripts/              # apply_ryazhenka_patches.sh, restore_protected.sh, protected_paths.txt
├── source/               # main.cpp, Lock.cpp/.hpp, Utils.hpp, Modes/, asmjit/, c4/, rapidyaml/
├── tester/               # вспомогательная утилита для отладки патчей
├── ExtractTitleids.py    # генератор titleids_with_patches.bin
├── Makefile              # подключает libryazhahand/ryazhahand.mk
└── README.md
```

## Синхронизация с upstream

Workflow `sync-upstream.yml` подтягивает изменения из `masagrator/FPSLocker`, прогоняет их через скрипт `scripts/apply_ryazhenka_patches.sh` (заменяющий `ultrahand` → `ryazhahand`, `ULTR` → `RYZH`, фиксирующий версию и т. д.) и оставляет защищённые пути (`.github/`, `scripts/`, `.gitmodules`, `README.md`, `Makefile`) нетронутыми. Подробности — `docs/SYNC.md`.

## Версионирование

- Единственный источник версии — файл `.ryazhenka-version` в корне репозитория.
- При запуске `scripts/apply_ryazhenka_patches.sh` значение копируется в `APP_VERSION` в `Makefile`.
- CI-сборки (`build.yml`) дополнительно дописывают к версии `+ryazh.<shortsha>`, чтобы каждый артефакт был трассируем до коммита.
- При пуше тега `v<X.Y.Z>` (например `v3.3.3`) `release.yml` форсит `APP_VERSION = <X.Y.Z>` и публикует GitHub Release.

Чтобы поднять версию: отредактируйте `.ryazhenka-version`, закоммитьте, поставьте тег `vX.Y.Z`, запушьте тег.

## Благодарности

Спасибо:
- ~WerWolv за создание Tesla-окружения
- ~ppkantorski за libultrahand / Ultrahand-Overlay (база для libryazhahand)
- ~masagrator — оригинальный автор FPSLocker
- ~cucholix + ~Monked за тесты
- ~CTCaer за инфу про Samsung OLED-панели
- ~NaGa за тесты на Retro Remake SUPER5 и предоставленный Switch OLED
  - и бекерам, включая: Jorge Conceição, zany tofu, Lei Feng, brandon foster, AlM, Alex Haley, Stefano Bigio, Le Duc, Sylvixor x
- Анонимному контрибьютору, нашедшему способ разблокировать полный диапазон частот для Switch OLED в доке.

Переводы:
- Немецкий: ~Lightos_
- Французский: ~ganonlebucher
- Русский: ~usagi, ~redraz
- Бразильский португальский: ~Fl4sh
- Китайский упрощённый: ~Soneoy, ~Tone Darkwell
- Китайский традиционный: [david082321](https://github.com/david082321)

## Sync Wait — список совместимых игр

В этих играх можно отключить vsync двойного буфера, выключив Window Sync Wait в FPSLocker:
- Batman - The Telltale Series (Warehouse-патч сразу включает triple buffer, опция не нужна)
- Pokémon Legends: Arceus
- Pokémon Legends: Z-A
- Pokémon Scarlet
- Pokémon Violet
- Sonic Frontier
- The Legend of Zelda: Tears of the Kingdom (Warehouse-патч сразу включает triple buffer, опция не нужна)
- Xenoblade Chronicles: Definitive Edition
- Xenoblade Chronicles 2
- Xenoblade Chronicles 3
- Xenoblade Chronicles X

## Лицензия

Сохранена оригинальная лицензия `masagrator/FPSLocker` — см. `LICENSE`.
