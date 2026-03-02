using UnityEngine;

public class AsteroidSpawner : MonoBehaviour
{
    public Transform plane;                 // 飞机

    [Header("Spawn Area")]
    public float rangeX = 5.33f;
    public float rangeY = 3f;
    public float minZ = 30f;
    public float maxZ = 80f;

    [Header("Spawn Control")]
    public int maxAsteroids = 30;
    public float spawnInterval = 0.5f;

    [Header("Asteroid Prefabs")]
    public string asteroidResourcePath = "Prefabs/Asteroid";

    float timer;
    GameObject[] asteroidPrefabs;

    void Start()
    {
        // 一次性加载所有陨石 Prefab
        asteroidPrefabs = Resources.LoadAll<GameObject>(asteroidResourcePath);

        if (asteroidPrefabs.Length == 0)
        {
            Debug.LogError("❌ 未在 Resources/" + asteroidResourcePath + " 中找到陨石 Prefab！");
        }
    }

    void Update()
    {
        if (plane == null || asteroidPrefabs == null || asteroidPrefabs.Length == 0)
            return;

        timer += Time.deltaTime;
        if (timer >= spawnInterval)
        {
            timer = 0f;
            TrySpawnAsteroid();
        }
    }

    void TrySpawnAsteroid()
    {
        // 控制场景中陨石数量
        if (GameObject.FindGameObjectsWithTag("Asteroid").Length >= maxAsteroids)
            return;

        Vector3 spawnPos = new Vector3(
            Random.Range(-rangeX, rangeX),
            Random.Range(-rangeY, rangeY),
            plane.position.z + Random.Range(minZ, maxZ)
        );

        // 随机选择一种陨石
        GameObject prefab = asteroidPrefabs[
            Random.Range(0, asteroidPrefabs.Length)
        ];

        GameObject asteroid = Instantiate(
            prefab,
            spawnPos,
            Random.rotation
        );

        // 随机缩放
        float scale = Random.Range(0.6f, 1.4f);
        asteroid.transform.localScale = Vector3.one * scale;
    }
}