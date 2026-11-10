# Повторная квалификация доступа к native VoN — 2026-11-04

**NO_QUALIFIED_ALLOWED_VON_POLICY_ACCESS.** Полная цель сохраняется. Этот проход перепроверил внешний блокер и не добавил работающего управляющего механизма.

1. Текущие опубликованные BI API четырёх классов не дали нового source-authorization/radio-population/final-q hook: [официальная поверхность](official/RESULT_RU.md), [VoNComponent BI](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceVoNComponent.html).
2. В промежуточном отчёте API роль не смогла проверить current HEAD. Root затем выполнил read-only `git ls-remote`: официальный HEAD/main **3d77cc212d5cda9922daf5f45635c7300d2d4cce** совпал с исходным frozen pin. Эта неопределённость снята: [root receipt](script-diff-head.json). Дата публикации страниц Doxygen не назначается по версии генератора.
3. InterceptAR master **b28a1e4310d6d066673af8e608578bce3e7b33f1** и README совпали с ранее обследованными. Повторного сканирования21 файлов не было. [Проверка native carrier](native/RESULT_RU.md).
4. Его latest остаётся Alpha8 с заявлением о Reforger1.7 Experimental; установленная game/Workbench версия1.8.0.13. Совместимость host и доступ именно к VoN policy не подтверждены. [Авторский релиз](https://github.com/intercept/interceptAR/releases/tag/Alpha8).

Общая регистрация C++ proto/reflection не предоставляет сама по себе управление получателями голоса. Отсутствие нужной операции в обследованной поверхности не доказывает невозможность других реализаций или скрытых host hooks; подтверждённого пригодного механизма в текущем проекте нет.

Необходимый внешний результат: конкретный совместимый интерфейс exact sender→TX binding и server radio population/final quality до overlay. Его надо квалифицировать до рабочего адаптера. Generic messages, общий mute, изменение одного range и вновь проверенный hidden-carrier вариант этот контракт не заменяют.

Текущий допуск владельца — только Script Editor compilation. DLL install, binary changes, Play/server/capture не разрешены. Новые запуски, изменения источников, DLL downloads и обход ограничений не выполнялись. Испытание A/B/C и DIRECT остаётся незакрытым.

Предыдущий goal turn был progress: новая raw-ID/RF проверка исключила конкретный вариант. Этот turn — повторная квалификация того же блокера без implementation progress. Цель не завершена, не сужена и пока остаётся active; это второе последовательное наблюдение отсутствия квалифицированного разрешённого voice-policy доступа.
