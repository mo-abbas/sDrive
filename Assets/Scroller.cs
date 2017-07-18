using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Scroller : MonoBehaviour {
    public float scrollSpeed = 0.4F;
    public Renderer rend;

    int scrolling = 0;

    void Start()
    {
        rend = GetComponent<Renderer>();
    }

    void Update()
    {
        float offset = Time.time * scrollSpeed * scrolling;
        rend.material.SetTextureOffset("_MainTex", new Vector2(0, offset));        
    }

    public void startScrolling()
    {
        scrolling = 1;     
    }

    public void stopScrolling()
    {
        scrolling = 0;     
    }
}
