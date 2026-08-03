# KieTrackerFirst 引き継ぎ資料

## 1. 目的、利用者、現在の機能

本プロジェクトは、Seeed Studio XIAO ESP32-S3とLSM6DSVモジュールを使用し、
VRChatの長時間プレイに対応する小型無線SlimeVRトラッカーを開発するものである。
リポジトリはSlimeVR Tracker ESPファームウェアを基礎としており、専用ボード定義、
LSM6DSVの起動時検証、安全なI²Cスキャン、ハードウェア手順書を追加済みである。

想定利用者は、組み立て済みトラッカーを受け取り、最後にバッテリーパックを組み込む
VRChatプレイヤーである。バッテリーの機構設計と電圧測定回路は未確定である。

## 2. 技術構成と各ファイルの責務

- PlatformIOとArduino-ESP32でファームウェアをビルドする。
- `platformio.ini`、`board-defaults.json`、`board-defaults.schema.json` が
  XIAO ESP32-S3の環境と生成されるボード設定を定義する。
- `src/sensors/softfusion/drivers/lsm6dsv.h` がLSM6DSVの識別情報と
  レジスタ設定を保持する。
- `src/sensorinterface` が診断情報付きI²Cレジスタ読み出しを担当する。
- `lib/i2cscan` が安全なデバイス検出とバス復旧を担当する。
- `lib/i2cscan/i2c_safety.h`、`include/sensor_probe.h`、
  `include/sensor_address_resolver.h` が、ビルド時に検証可能な純粋ルールを保持する。
- `docs/xiao-esp32s3-lsm6dsv.md` が、この構成における配線と実機テスト手順の
  authoritative sourceである。

## 3. 不変条件と情報源の優先順位

この構成では、SDA GPIO5、SCL GPIO6、アドレス `0x6B`、`WHO_AM_I`
レジスタ `0x0F`、期待値 `0x70`、製品用I²Cクロック100kHzを維持する。
I²Cスキャン処理は `0x08`～`0x77` の範囲外へ送信してはならない。

情報源の優先順位は次のとおりである。

1. プロジェクト所有者が提示した実機確認結果
2. STおよびSeeed Studioの公式資料
3. SlimeVR公式ドキュメント
4. モジュール販売ページおよび組み立て手順
5. 既存コード内の仮定

下位の情報源を根拠として、確定済みの実機結果を上書きしてはならない。

## 4. 未確定、失敗、空状態の意味

- `ADDRESS_NACK`: デバイスがアドレスへ応答しなかった状態（`tx=2`）。
- `TRANSMISSION_ERROR`: 2以外の非ゼロ送信結果。
- `READ_FAILURE`: 送信は成功したが、要求したバイト数を受信できなかった状態。
- `WHO_AM_I_MISMATCH`: 読み出しは完了したが、値が `0x70` ではなかった状態。
- 上記4状態は既存ネットワークプロトコル上では `SENSOR_ERROR` へ集約し、
  詳細な原因はシリアルログへ残す。SlimeVRのパケット形式は変更していない。
- プルアップ抵抗値、バッテリー設計、400kHz試験結果などの未知値を、ゼロ、
  ハードウェアなし、または合格済みとして表現してはならない。

## 5. 保存、キャッシュ、スキーマ

新しい永続データやネットワークキャッシュは追加していない。
ボード既定値のJSON Schemaへ `BOARD_XIAO_ESP32S3` を追加済みであり、
`scripts/preprocessor.py` がボード用マクロ生成のauthoritative sourceである。

## 6. 外部通信、セキュリティ、プライバシー

外部通信は上流SlimeVRファームウェアの既存動作から変更していない。
テレメトリ、認証情報、個人情報、新規外部サービスは追加していない。
Wi-Fi認証情報を `platformio.ini` やソースコードへコミットしてはならない。

## 7. ライセンスとクレジット

既存プロジェクトのMIT/Apache-2.0デュアルライセンス方針を維持する。
今回の変更では第三者ソースやバイナリアセットを追加していない。
参照したハードウェアおよび文書の権利は、それぞれの権利者に帰属する。

## 8. UXとアクセシビリティ

新しいGUIはない。診断シリアルログは、色だけに依存せず、状態名とI²Cの数値情報で
結果を伝える。

## 9. セットアップ、開発、テスト、ビルド、リリース

VS CodeのPlatformIOから次のタスクを実行する。

```text
PlatformIO: Build -> BOARD_XIAO_ESP32S3
PlatformIO: Build -> BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC
PlatformIO: Test  -> BOARD_XIAO_ESP32S3（契約テスト＋実機テスト）
```

`test_i2c_contract` スイートには15件のコンパイル時アサーションがあり、XIAO用
テストファームウェアのビルド時に検証される。`test_xiao_lsm6dsv_hardware`
スイートの実行にはトラッカー実機が必要である。ファームウェア書き込みと実機テストは
VS CodeのPlatformIOから行う。コンパイル成功だけを根拠として、電源再投入、400kHz、
8時間連続試験を合格扱いしてはならない。

このワークスペースで使用しているVS Code PlatformIO Coreによる統合検証結果：

- `BOARD_XIAO_ESP32S3` ファームウェアビルド: PASS。RAM
  45,168/327,680 bytes、Flash 1,148,237/3,342,336 bytes。
- `BOARD_XIAO_ESP32S3_400KHZ_DIAGNOSTIC` ファームウェアビルド: PASS。
  RAM 45,168/327,680 bytes、Flash 1,148,345/3,342,336 bytes。
- 製品環境のテストファームウェア・コンパイル: PASS。2スイートを収集・ビルド。
- 400kHz診断環境のテストファームウェア・コンパイル: PASS。
  2スイートを収集・ビルド。
- 共有コードの回帰ビルド: `BOARD_WEMOSD1MINI` PASS、
  `BOARD_XIAO_ESP32C3` PASS。
- 契約テストの15件の `static_assert` を両環境でコンパイル済み。
- Unity実行時テスト: 未実施。`--without-uploading --without-testing` を使用し、
  実機へ書き込んでいないため。
- 両ビルドのボード生成ログで、SDA 5、SCL 6、`IMU_LSM6DSV`、address 107、
  INT 255、`BAT_INTERNAL` を確認済み。
- `git diff --check`: PASS。
- このWindows環境では単体の `clang-format` を利用できなかったため、
  フォーマッタコマンドは未実施。

リリースには、自動検証、100kHz実機試験、筐体・バッテリー安全性の確定、
対象差分がクリーンであることが必要である。push、merge、公開、deployは明示的な
許可を得てから行う。

## 10. Git、ブランチ、DEV/MERGEの役割

- 実装開始時の作業ブランチ: `xiao-lsm6dsv`
- 分岐元および実装開始時の先端: `5e680f7`（確認時点の `origin/main`）
- DEV: 実装、文書、テスト、関連コミットを担当する。
- MERGE: 統合テストと、明示的に許可されたpushのみを担当する。

未追跡の `test/I2C_TEST.cpp` は今回の作業以前から存在し、変更していない。
このファイルは `0x01`～`0x7E` を走査するため、このモジュールには安全ではない。
実行、ステージ、上書き、保守対象プローブとしての利用をしてはならない。
保守対象は `pio-test/test_xiao_lsm6dsv_hardware` である。

他のdetached worktreeにはユーザー所有の状態が存在する。それらの未コミット変更は
今回の統合に含めていない。

当初のnative testは、このWindows環境のPATHにホスト用 `gcc/g++` が無く、さらに
PlatformIOが `test/` 直下の旧ファイルを共有テストソースとして扱ったため中止した。
保守対象スイートを専用の `pio-test/` へ移し、決定的な検証を組み込みC++20の
コンパイル時アサーションへ変更した。ホスト用コンパイラの追加インストールは不要である。

## 11. 並行作業で衝突しやすい領域

ボード番号、ボード既定値JSON、`platformio.ini`、共通SoftFusion起動経路、共通I²C
スキャナは複数ハードウェアへ影響する。非XIAOボードの起動クロックを変更せず、変更後は
回帰ビルドを実行する。

## 12. テストの真実性を守る規則

未実施の実機テストを合格扱いしない。PlatformIO環境、テストスイート、合否件数、
skip、接続した実機、I²Cクロックを記録する。ビルド成功が証明するのはコンパイル可能性
だけである。安全スキャナのテストでは、0x7Eを表示しなかっただけでなく、実際に
0x7Eへ送信していないことを確認する。

## 13. リリース条件と手順

リリース前に次を完了する。

- 契約テストと製品ファームウェアビルドを通す。
- 100kHzで完全な電源遮断・再投入を10回行う。
- バッテリーパック、保護回路、コネクタ、ADC、筐体を確定する。
- SlimeVR Serverへ接続した8時間連続試験を完了する。
- 秘密情報、生成物、ライセンス、最終Git差分を確認する。

400kHzの結果は参考情報であり、100kHz製品版のリリース条件にはしない。

## 14. 実測した技術知識

```text
--- WHO_AM_I direct read ---
0x6B -> WHO_AM_I = 0x70 [OK: LSM6DSV]
--- safe I2C scan ---
found: 0x6B
```

この実機結果により、対象個体のモジュール本体、3.3V電源、GPIO5/GPIO6配線、
アドレス0x6B、100kHzでのWHO_AM_I読み出しが正常であることを確認した。
`0x01`～`0x7E` の広範囲スキャンでは0x6Bに加えて0x7Eも検出され、その後の
WHO_AM_I読み出しが失敗した。0x7Eは別のI²C周辺機器ではなく、I3Cの予約・
ブロードキャスト用アドレスとして扱う。

## 15. 却下した案と理由

- LSM6DSVの既定値を `0x6A` にする案: SA0 Highで0x6Bとなる実機結果に反する。
- 代替アドレスを `0x6B + 1` で生成する案: 無効な0x6Cになる。
  代替アドレスは明示的に0x6Aとする。
- `0x01`～`0x7E` または0x7Fまで走査する案: I3C予約アドレス0x7Eへ送信し、
  後続の読み出しを実際に阻害したため。
- 現時点で400kHzを製品設定にする案: 組み立て済み実機で未確認のため。

## 16. C/A/U、対象外、次のタスク

`C`（確定）：3.3V、SDA GPIO5、SCL GPIO6、SA0 High、アドレス `0x6B`、
`WHO_AM_I` の `0x0F == 0x70`、100kHz実機成功、安全でないスキャンの失敗履歴。

`A`（仮決定）：電源安定待ち500ms、製品設定100kHz、INT未使用、取付回転は暫定
`DEG_0`、バッテリー回路確定までESP32の測定を `BAT_INTERNAL` で無効相当とする、
詳細なエラー原因はシリアルログのみで通知する。

`U`（未決定）：400kHzの製品適合性、モジュール上のプルアップ抵抗値、最終INT配線、
取付回転、バッテリーセル、保護回路、コネクタ、充電電流との適合性、ADC分圧、筐体、
長時間動作の最終結果。

今回の統合対象外：バッテリー購入、筐体CAD、製品リリース、push、deploy、
未追跡の旧プローブ変更。

次のタスク：100/400kHz実機比較、完全な電源再投入10回、センサ切断時のエラーログ取得、
8時間SlimeVR連続試験、バッテリー・筐体の確定。

## 17. BATON

```text
ブランチ: xiao-lsm6dsv
先端: HEAD（このHANDOFFを含むコミット。実値は `git rev-parse --short HEAD` で確認）
分岐元: origin/main 5e680f7
概要: XIAO ESP32-S3/LSM6DSVの0x6B・100kHz起動、安全走査、診断、文書を統合
テスト: firmware build 2/2 PASS、test-firmware compile 4/4 suites PASS、15件の契約assertを両環境で評価、実機Unity/power-cycle/8時間は未実施
注意点: test/I2C_TEST.cppは所有者不明の未追跡・危険な旧スキャナのため未変更。pushなし
```
# 2026-07-30 defines.h-equivalent target configuration

The repository now expresses the chapter-07 settings through the generated
`BOARD_XIAO_ESP32S3` defaults instead of editing the global `src/defines.h` or
falling back to `BOARD_CUSTOM`. The target generates LSM6DSV at `0x6B` with
`DEG_0`, SDA/SCL/INT/AUX-INT/battery pins 5/6/4/2/1, and USB-powered
`BAT_INTERNAL` with resistance values 180/100/220. The INT assignments remain
physically unconnected and unused; GPIO1 and the resistor values do not validate
a battery circuit.

PlatformIO Core 6.1.19 compiled production and 400 kHz diagnostic firmware,
the WEMOS D1 Mini and XIAO ESP32-C3 regressions, and four test-firmware
configurations. The test command used `--without-uploading --without-testing`;
therefore zero Unity runtime or hardware cases were executed.
