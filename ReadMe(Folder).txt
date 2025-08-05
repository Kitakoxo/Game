2025/8/5 フォルダ分け

Source
├─ AI                 // 敵・NPCのAI処理
├─ Audio              // BGM・SEなどの音響処理
├─ Camera             // カメラ制御
├─ Core               // ゲームの基盤処理（エントリーポイント・共通処理など）
├─ Effect             // パーティクルやポストエフェクトなどの視覚効果
├─ Entity             // キャラクターやオブジェクトのロジック（プレイヤー・敵など）
├─ Graphics           // グラフィックス関連（APIレベルの処理やリソース管理）
├─ Input              // 入力処理（キーボード・ゲームパッドなど）
├─ Model              // モデル構造・読み込み（GLTFImporter含む）
├─ Physics            // 物理処理（Collider, Solver に細分）
│  ├─ Collider
│  └─ Solver
├─ Renderer           // 描画処理の機能別分割
│  ├─ Model           // モデル描画用
│  ├─ Shape           // プリミティブ図形描画用
│  └─ Sprite          // 2Dスプライト描画用
├─ Scene              // シーン管理（ベース、ゲーム本編、デバッグなど）
│  ├─ Base
│  ├─ Debug
│  ├─ Flow
│  └─ Game
├─ Tools              // 開発用ツール（ノードエディタなど）
│  └─ NodeEditor
├─ UI                 // UI描画・管理
│  └─ ImGui           // ImGui系UI制御（+Dialog系）
├─ Main.cpp           // メイン関数（ゲームエントリーポイント）
