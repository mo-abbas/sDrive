using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Scroller : MonoBehaviour {
    public float scrollSpeed = 0.4F;
    public Renderer rend;

    void Start()
    {
        rend = GetComponent<Renderer>();
    }

    void Update()
    {
        float offset = Time.time * scrollSpeed;
        rend.material.SetTextureOffset("_MainTex", new Vector2(0, offset));

        if (Input.GetKey(KeyCode.Backspace))
            UnityEngine.SceneManagement.SceneManager.LoadScene("menu");
    }

}
