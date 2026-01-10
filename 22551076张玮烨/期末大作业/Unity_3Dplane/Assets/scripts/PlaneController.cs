using System.Collections;
using UnityEngine;

public class PlaneController : MonoBehaviour
{

    [Header("Explosion")]
    public GameObject explosionPrefab;
    [Header("Engine Effect")]
    public ParticleSystem engineParticles;
    public Color normalEngineColor = new Color(0.4f, 0.8f, 1f);
    public Color damagedEngineColor = new Color(1f, 0.4f, 0.1f);
    public float recoverSpeed = 2f;

    ParticleSystem.MainModule engineMain;
    bool isDamaged = false;


    [Header("Movement")]
    public float moveSpeed = 6f;
    public float baseForwardSpeed = 10f;
    public float boundaryX = 5.33f;
    public float boundaryY = 3f;

    [Header("Speed Increase")]
    public float speedIncreasePerUnit = 0.02f;
    public float maxForwardSpeed = 30f;

    [Header("Tilt")]
    public float tiltAngle = 25f;
    public float tiltSpeed = 5f;

    float startZ;
    bool isDead = false;


    void Start()
    {
        startZ = transform.position.z;

        if (engineParticles != null)
        {
            engineMain = engineParticles.main;
            engineMain.startColor = normalEngineColor;
        }
    }


    void Update()
    {
        if (isDead) return;

        float h = Input.GetAxis("Horizontal");
        float v = Input.GetAxis("Vertical");


        float distance = transform.position.z - startZ;
        float forwardSpeed =
            baseForwardSpeed + distance * speedIncreasePerUnit;
        forwardSpeed = Mathf.Min(forwardSpeed, maxForwardSpeed);

        transform.Translate(Vector3.forward * forwardSpeed * Time.deltaTime, Space.World);


        Vector3 move = new Vector3(h, v, 0f);
        transform.Translate(move * moveSpeed * Time.deltaTime, Space.World);

        ClampPosition();
        ApplyTilt(h, v);
        UpdateEngineColor();
    }


    void ClampPosition()
    {
        Vector3 pos = transform.position;
        pos.x = Mathf.Clamp(pos.x, -boundaryX, boundaryX);
        pos.y = Mathf.Clamp(pos.y, -boundaryY, boundaryY);
        transform.position = pos;
    }

    void ApplyTilt(float h, float v)
    {
        float roll = -h * tiltAngle;
        float pitch = v * tiltAngle;

        Quaternion target = Quaternion.Euler(pitch, 0f, roll);
        transform.rotation = Quaternion.Slerp(
            transform.rotation,
            target,
            tiltSpeed * Time.deltaTime
        );
    }


    void UpdateEngineColor()
    {
        if (!isDamaged || engineParticles == null) return;

        Color current = engineMain.startColor.color;
        Color target = normalEngineColor;

        Color next = Color.Lerp(
            current,
            target,
            recoverSpeed * Time.deltaTime
        );

        engineMain.startColor = next;

        if (Vector4.Distance(next, target) < 0.05f)
        {
            engineMain.startColor = normalEngineColor;
            isDamaged = false;
        }
    }

    public void OnDamaged()
    {
        if (engineParticles == null) return;

        engineMain.startColor = damagedEngineColor;
        isDamaged = true;
    }

    public void Explode()
    {
        if (isDead) return;
        isDead = true;

        StartCoroutine(ExplodeRoutine());
    }

    IEnumerator ExplodeRoutine()
    {
        float explodeTime = 0.6f;


        if (explosionPrefab != null)
        {
            GameObject exp = Instantiate(
                explosionPrefab,
                transform.position,
                Quaternion.identity
            );

            ParticleSystem ps = exp.GetComponent<ParticleSystem>();
            if (ps != null)
                explodeTime = ps.main.duration;
        }


        DisablePlane();


        yield return new WaitForSecondsRealtime(explodeTime);


        GameOverManager gm = FindObjectOfType<GameOverManager>();
        if (gm != null)
        {
            gm.ShowGameOver(transform.position.z);
        }
    }


    void DisablePlane()
    {

        foreach (Renderer r in GetComponentsInChildren<Renderer>())
            r.enabled = false;


        Collider col = GetComponent<Collider>();
        if (col != null) col.enabled = false;

        if (engineParticles != null)
            engineParticles.Stop();
    }
}
