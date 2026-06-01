# Upstream sync — `masagrator/FPSLocker`

> **EN (TL;DR).** This fork tracks `masagrator/FPSLocker` as upstream while
> building against `libryazhahand` instead of `libtesla`/`libultrahand`. The
> `Sync upstream` GitHub Actions workflow merges upstream changes weekly (or on
> demand), restores all Ryazhenka-protected files from `main`, re-applies our
> rename/signature/version patches, and opens a PR for review. Protected paths
> (Makefile, .github/, scripts/, README.md, .gitmodules, docs/, libryazhahand
> submodule) are never overwritten by an upstream sync.

---

## Что делает sync-workflow

Файл `.github/workflows/sync-upstream.yml` запускается:

- по расписанию — каждый понедельник в 04:17 UTC;
- вручную через `workflow_dispatch` (вкладка Actions → Sync upstream → Run workflow).

Параметры ручного запуска:

| Параметр | По умолчанию | Назначение |
|----------|--------------|-----------|
| `upstream` | `masagrator/FPSLocker` | какой репозиторий считаем upstream |
| `upstream_branch` | `main` | какую ветку оттуда тащим |
| `target_branch` | `main` | в какую ветку этого репо потом будет PR |

## Пайплайн

1. **Checkout** локальной целевой ветки.
2. **Add upstream remote** + `git fetch upstream <branch>`.
3. **Create sync branch** `sync/upstream-YYYYMMDD-HHMMSS`.
4. **Merge upstream** с `-X theirs` (на конфликтах берём upstream-версию).
5. **Restore protected** — `scripts/restore_protected.sh` возвращает файлы из списка `scripts/protected_paths.txt` из ветки `main`.
6. **Apply Ryazhenka patches** — `scripts/apply_ryazhenka_patches.sh`:
   - APP_VERSION ← `.ryazhenka-version` (сейчас `3.3.3`);
   - `libultrahand` → `libryazhahand`, `Ultrahand-Overlay` → `Ryazhahand-Overlay` в любых *.md/*.txt вне защищённых директорий;
   - в `Makefile`: восстанавливается include блока `libs/libryazhahand/ryazhahand.mk` (с фолбэком на `ultrahand.mk`), подпись `ULTR` → `RYZH`;
   - `.gitmodules` перезаписывается на `libryazhahand`;
   - удаляются случайно затащенные `libs/libultrahand` или `libs/libtesla`.
7. **Commit + push** на ветку `sync/upstream-...`.
8. **Open PR** в `target_branch` (если есть диф).

## Если PR не создаётся (`Permission denied`)

`GITHUB_TOKEN` по умолчанию в репозитории **не** имеет прав создавать PR. Если в логе sync-workflow вы видите:

```
GitHub Actions is not permitted to create or approve pull requests. (403)
```

Это надо включить **один раз** в настройках репозитория:

> **Settings → Actions → General → Workflow permissions →** галочка
> **«Allow GitHub Actions to create and approve pull requests»** → Save.

После этого следующий запуск sync-workflow откроет PR автоматически.

До тех пор workflow не падает — он пушит sync-ветку на `origin`, в Summary запуска оставляет ссылку вида `…/compare/main...sync/upstream-…?expand=1`, и PR можно открыть в один клик руками. Если PR на эту ветку уже открыт — workflow ничего не делает.

## Защищённые пути

Список — `scripts/protected_paths.txt`:

```
.github/
scripts/
docs/
.gitmodules
.gitignore
README.md
Makefile
libs/libryazhahand
```

Любой файл по этим путям после merge upstream откатывается к версии целевой ветки. Если хочется что-то ещё «забронировать» — допишите строку в `protected_paths.txt`.

## Если sync пришёл с конфликтами по исходникам

`masagrator/FPSLocker` собирается под `libtesla`, наш форк — под `libryazhahand` (форк `libultrahand`). Большие изменения в `source/*.cpp/*.hpp` могут не подходить по API.

PR из sync-workflow стоит читать вручную:

- проверить, что `Makefile`, `.gitmodules`, `.github/`, `scripts/`, `README.md` не сломались (они защищены);
- проверить, что в `source/` нет вызовов API из `libtesla`, которые отсутствуют в `libryazhahand`;
- при необходимости — оставить часть upstream-изменений, остальные откатить.

## Запуск pipeline вручную локально

```bash
# из чистой ветки sync/...
git remote add upstream https://github.com/masagrator/FPSLocker.git
git fetch upstream main
git merge --no-ff --no-edit -X theirs upstream/main

bash scripts/restore_protected.sh main
bash scripts/apply_ryazhenka_patches.sh

git add -A
git commit -m "sync: re-apply Ryazhenka customisations"
```

## Где меняется версия

- `.ryazhenka-version` — единственный источник истины. `apply_ryazhenka_patches.sh` берёт значение оттуда и проставляет в `Makefile`.
- workflow `release.yml` при пуше тега `v3.3.3` дополнительно пересинхронизирует `APP_VERSION` с тегом.
- workflow `build.yml` для не-релизных сборок дописывает к версии `+ryazh.<shortsha>`, чтобы каждый артефакт был трассируем до коммита.
