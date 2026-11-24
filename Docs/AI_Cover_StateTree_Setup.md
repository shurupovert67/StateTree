# AI «Стрельба из-за укрытия» — как всё устроено и как запустить

## Быстрый старт (после пересборки проекта)

1. Открыть проект `StateTree.uproject` (если редактор был открыт — перезапустить, чтобы он увидел новые ассеты).
2. Открыть уровень **`/Game/AI/Lvl_CoverShooting`**.
3. Нажать **Play**.

Бот сам: выберет укрытие → дойдёт до него → присядет → встанет → даст очередь из 4 снарядов →
сядет обратно → и так по кругу. Если игрок обойдёт укрытие — бот выберет другое.

Готовые ассеты (созданы кодом, руками ничего собирать не нужно):

| Ассет | Что это |
|---|---|
| `/Game/AI/StateTree/ST_Bot_CoverShooter` | само дерево состояний (schema `StateTree AI Component`) |
| `/Game/AI/Lvl_CoverShooting` | демо-уровень: пол из шаблона + 5 укрытий + бот + NavMeshBoundsVolume |

Пересобрать всё одной командой (например, после изменения C++-задач):

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Users\user\Documents\Unreal Projects\StateTree\StateTree.uproject" `
  -run=pythonscript -script="C:\Users\user\Documents\Unreal Projects\StateTree\Scripts\build_cover_demo.py" `
  -unattended -nullrhi -nosplash
```

Или из консоли редактора: `py "<Project>/Scripts/build_cover_demo.py"`.

Скрипт вызывает `UCoverAIEditorTools` (модуль `CoverAIEditor`):
- `BuildCoverStateTree()` — создаёт/пересоздаёт дерево, компилирует и сохраняет;
- `BuildDemoLevel()` — копирует карту-шаблон, чистит старых ботов/укрытия, ставит 5 укрытий,
  бота и NavMeshBoundsVolume, собирает навмеш и сохраняет как `Lvl_CoverShooting`.

---

## Почему «ничего не работало»

Из лога (`Saved/Logs/StateTree.log`) причина была однозначной:

```
LogCoverAI: Error: [...] StateTree asset '/Game/AI/StateTree/ST_Bot_CoverShooter' was not found.
LogStateTree: Error: UStateTreeComponent::ValidateStateTreeReference: The State Tree asset is not set.. Cannot initialize.
```

То есть ассета дерева не было — боту нечего было исполнять. Второй проблемой был **отсутствующий
навмеш** на уровне: без него `AI MoveTo` не может дойти до укрытия (бот выбирал бы укрытие и застревал).

Типичные причины «тишины»:

| Симптом в логе | Причина |
|---|---|
| `State Tree asset is not set` | нет ассета по пути `/Game/AI/StateTree/ST_Bot_CoverShooter` |
| `No suitable cover found` каждую секунду | нет навмеша / укрытия стоят не той стороной к игроку |
| `MoveToCover: movement stopped ... away` | бот не может дойти (навмеш/препятствие) |
| в мире 2+ `CoverShooterController` | контроллер добавлен в уровень руками (его создаёт сам бот) |

Диагностика: Output Log, фильтр `LogCoverAI`, подробно — `Log LogCoverAI Verbose`.
Включить подробный лог заранее: `-LogCmds="LogCoverAI Verbose"`.

---

## Что в C++ (структура для защиты работы)

```
Source/
  CoverAI/                        — геймплей (Runtime)
    Cover/CoverPoint              — блок-укрытие 200×200×130, точка «за блоком» считается от игрока
    Cover/CoverShooterCharacter   — бот: присед, прицел, выстрел, выбор укрытия, проверка LOS
    Cover/CoverProjectile         — снаряд (прямой полёт, damage, self-ignore)
    Cover/CoverShooterController  — AIController + StateTreeAIComponent (сам грузит дерево)
    StateTree/STT_* , STC_*       — задачи и условие для StateTree
  CoverAIEditor/                  — editor-only утилиты, которые собирают ассеты
```

Состояния дерева (5 штук, по одному на шаг задания):

| Состояние | Задача | Переход |
|---|---|---|
| `FindBestCover` | `Cover: Find Best Cover` | Succeed → `MoveToCover`, Fail → `Wait` |
| `MoveToCover` | `Cover: Move To Cover` | Succeed → `InCover`, Fail → `Wait` |
| `InCover` | `Cover: Set Crouch` (Hold 0.6 c) | Completed → `PeekAndShoot`; On Tick + `Cover: Needs New Cover` → `FindBestCover` |
| `PeekAndShoot` | `Cover: Peek And Shoot` (4 выстрела, 0.3 c) | Completed → `InCover` |
| `Wait` | встроенная `Delay Task` (1 c) | Completed → `FindBestCover` |

Как выбирается укрытие (`ACoverShooterCharacter::SelectBestCover`):

1. укрытие отбрасывается, если игрок **не** с фронтальной стороны блока;
2. точка за блоком проецируется на навмеш;
3. дистанция «точка → игрок» должна быть в `[MinFiringDistance, MaxFiringDistance]`;
4. линия «глаза игрока → точка» должна быть **перекрыта** (`LineTrace ECC_Visibility`);
5. счёт = `дистанция_бот→точка + 0.5·|дистанция_игрок→точка − IdealFiringDistance|`, побеждает минимум.

---

## Тюнинг (Details бота / задачи)

| Параметр | Смысл |
|---|---|
| `MinFiringDistance` / `MaxFiringDistance` | допустимая дистанция до игрока из укрытия (600 / 3500) |
| `IdealFiringDistance` | комфортная дистанция (1200) |
| `FiringDistanceWeight` | вес «дистанции до игрока» в счёте (0.5) |
| `bDrawCoverDebug` | рисовать выбранную точку (зелёная сфера + линия) |
| `AimSpreadDegrees`, `AimInterpSpeed` | разброс и скорость доворота |
| `bUsePlaceholderCrouchVisual`, `CrouchVisualScaleZ` | «присед» без crouch-анимаций: меш прижимается к полу и сплющивается по Z |
| `ShotsPerBurst`, `ShotInterval`, `StandUpDelay` | темп стрельбы в задаче `Peek And Shoot` |
| `HoldTime` у `Cover: Set Crouch` | сколько сидеть в укрытии перед выходом |

Блок по умолчанию 130 см: он скрывает присевшего бота и при этом ниже дула (143 см), чтобы
стрелять стоя. Если уменьшить блок, присед перестанет скрывать бота.

---

## Если хочешь использовать свой уровень (`Lvl_ThirdPerson`)

Сейчас в нём нет навмеша, и остались лишние актёры с прошлых попыток:

1. Удалить актёров `CoverShooterController` (их создаёт сам бот, в уровне они не нужны).
2. Удалить случайно поставленные `CoverProjectile`.
3. Place Actors → Volumes → **NavMesh Bounds Volume**, растянуть на всю арену, нажать `P`
   (должна быть зелёная сетка).
4. `CoverPoint` ставить так, чтобы **оранжевая стрелка смотрела в сторону игрока**
   (укрытие защищает с фронта).
5. Один `CoverShooterCharacter` — контроллер и дерево он подхватит сам.

---

## Ручная сборка дерева (справочно)

Если нужно собрать `ST_Bot_CoverShooter` руками (например, для отчёта): Add → Artificial
Intelligence → State Tree → schema **StateTree AI Component**; в состояниях добавить задачи
из группы `AI|Cover` (у каждой свойство `Pawn` с меткой `CONTEXT` — подставится автоматически),
переходы — как в таблице выше. Кнопка **Compile** должна быть зелёной.

---

## Проверено

- C++ модули `CoverAI` и `CoverAIEditor` компилируются и линкуются (`StateTreeEditor Win64 Development`).
- Дерево компилируется без ошибок (`StateTreeEditor: Compile StateTree ... succeeded`).
- Полный цикл проверен запуском `-game -nullrhi` на `Lvl_CoverShooting`:
  `FindBestCover → MoveToCover → SetCrouch(hold 0.60) → PeekAndShoot → 4× Shot fired → SetCrouch → ...`

## Известные ограничения

- Нет crouch-анимаций в шаблоне — используется «заглушка» (см. `bUsePlaceholderCrouchVisual`).
- Оружия визуально нет: снаряд летит из `MuzzlePoint`.
- Цель — `GetPlayerController(0)` (по заданию один игрок). Для мультиплеера — AI Perception.
- Навмеш на демо-уровне собран в редакторе, но при `-game` UE всё равно может пересобрать его
  при загрузке (в логе `SpawnMissingNavigationData ... rebuilding at runtime`) — это нормально.
