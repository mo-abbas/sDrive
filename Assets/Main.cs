using System;
using System.Linq;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Tuple<T1, T2>
{
    public T1 First { get; private set; }
    public T2 Second { get; private set; }
    internal Tuple(T1 first, T2 second)
    {
        First = first;
        Second = second;
    }
}

public struct Frame
{
    public int number;
    public List<float> leftRoadBorder;
    public List<float> rightRoadBorder;
}

public class Main : MonoBehaviour { 
    public Transform car_model;
    public Texture warning_image;

    public event System.EventHandler rendering_started;

    static List<Frame> Frames = new List<Frame>();
    static Dictionary<int, Dictionary<int, Vector2>> cars = new Dictionary<int, Dictionary<int, Vector2>>();
    static int carIDs = 1;
    static int currentFrame = 0;
    static Dictionary<int, GameObject> carsIG = new Dictionary<int, GameObject>();
    static Vector3[] vertices = new Vector3[4];

    static float video_width = 0.0f;
    static float video_height = 0.0f;
    static float video_fps = 0.0f;

    static bool display_threshold = false;

    string warningText = "A car is too near to you";
    bool warning = false;

    const float CAR_WIDTH = 2f;
    const float CAR_LENGTH = 4.64f;

    void Start () {
        parseJSON(System.IO.Path.Combine(Application.dataPath, @"..\Output\results.json"));        
    }

    void parseJSON(string path)
    {
        if (!System.IO.File.Exists(path))
            return;

        var json = System.IO.File.ReadAllText(path);

        video_width = float.Parse(((JObject)JToken.Parse(json)).SelectToken("input.width", true).ToString());
        video_height = float.Parse(((JObject)JToken.Parse(json)).SelectToken("input.height", true).ToString());
        video_fps = float.Parse(((JObject)JToken.Parse(json)).SelectToken("input.fps", true).ToString());

        var jFrames = ((JObject)JToken.Parse(json)).SelectToken("frames", true).Children();        

        foreach (JProperty p in jFrames)
        {
            int fNumber = int.Parse(p.Name);

            Frame f;
            f.number = fNumber;

            try
            {
                var jCarList = (JArray)(((JObject)p.Value)["cars"]);
                var frameCars = new List<Vector2>();

                foreach (var jCar in jCarList)
                {
                    var cCoords = jCar.ToString().Split(',');
                    var newCar = new Vector2(float.Parse(cCoords[0]), float.Parse(cCoords[1]));

                    if (frameCars.Count == 0)
                    {
                        frameCars.Add(newCar);
                    }
                    else
                    {
                        Vector2 oldCar = frameCars.Last();

                        if ((Math.Abs(newCar.x - oldCar.x) < CAR_WIDTH) &&
                            (Math.Abs(newCar.y - oldCar.y) < CAR_LENGTH))
                        {
                            frameCars[frameCars.Count - 1] = new Vector2((newCar.x + oldCar.x) / 2, (newCar.y + oldCar.y) / 2);
                        }
                        else
                        {
                            frameCars.Add(newCar);
                        }
                    }
                }

                foreach (var fCar in frameCars)
                {
                    if (fNumber == 1)
                    {
                        int id = carIDs++;
                        cars.Add(id, new Dictionary<int, Vector2>());
                        cars[id].Add(fNumber, fCar);
                    }
                    else
                    {
                        bool added = false;

                        float minDist = 1000;
                        int carID = 0;

                        foreach (var car in cars)
                        {
                            var lastValue = car.Value.Last();

                            if (lastValue.Key != (fNumber - 1))
                                continue;

                            float dist = Vector2.Distance(fCar, lastValue.Value);

                            minDist = Math.Min(minDist, dist);
                            if (dist == minDist)
                                carID = car.Key;
                        }

                        if (minDist <= 2F)
                        {
                            cars[carID].Add(fNumber, fCar);
                            added = true;
                        }

                        if (!added)
                        {
                            int id = carIDs++;
                            cars.Add(id, new Dictionary<int, Vector2>());
                            cars[id].Add(fNumber, fCar);
                        }
                    }
                }
            }
            catch (System.Exception) { }

            var rCoords = ((JObject)p.Value)["road"]["left"].ToString().Split(',');
            f.leftRoadBorder = new List<float>();
            for (int i = 0; i < rCoords.Length; i++)
                f.leftRoadBorder.Add(float.Parse(rCoords[i]));

            rCoords = ((JObject)p.Value)["road"]["right"].ToString().Split(',');
            f.rightRoadBorder = new List<float>();
            for (int i = 0; i < rCoords.Length; i++)
                f.rightRoadBorder.Add(float.Parse(rCoords[i]));

            Frames.Add(f);
        }

        //foreach (var car in cars)
        //{
        //    Debug.Log(car.Key + " " + car.Value.First().Key + "->" + car.Value.Last().Key + " " + car.Value.First().Value.ToString() + "->" + car.Value.Last().Value.ToString());
        //}  
    }

    float calculatePolynomial(List<float> coeffs, int value)
    {
        float result = 0;
        int order = coeffs.Count - 1;

        for (int o = 0; o <= order; o++)
            result += coeffs[o] * (float)Math.Pow(value, o);

        return result;
    }

    public void startRendering()
    {
        display_threshold = bool.Parse(new CSINI(System.IO.Path.Combine(Application.dataPath, @"..\config.ini")).IniReadValue("settings", "display_threshold"));

        vertices[0] = new Vector3(calculatePolynomial(Frames[0].rightRoadBorder, 20) + 2, 0, 20);
        vertices[1] = new Vector3(calculatePolynomial(Frames[0].rightRoadBorder, -20) + 2, 0, -20);
        vertices[2] = new Vector3(calculatePolynomial(Frames[0].leftRoadBorder, -20) - 2, 0, -20);
        vertices[3] = new Vector3(calculatePolynomial(Frames[0].leftRoadBorder, 20) - 2, 0, 20);

        InvokeRepeating("renderFrame", 0, 1 / video_fps);
    }

    void renderFrame()
    {
        if (currentFrame == 0)
        {
            GetComponent<Scroller>().startScrolling();

            if (rendering_started != null)
                rendering_started(this, null);
        }
        else if (currentFrame >= Frames.Count)
        {
            GetComponent<Scroller>().stopScrolling();
            CancelInvoke();
            return;
        }  

        Frame frame = Frames[currentFrame++];

        /* The vertex indicies look like this, with these triangles
         *         3 ------ 0
         *           |   /|
         *           |  / |
         *           | /  |
         *           |/   |
         *         2 ------ 1
         */
        vertices[0].x = 0.8f * vertices[0].x + 0.2f * (calculatePolynomial(frame.rightRoadBorder, 20) + 2);
        vertices[1].x = 0.8f * vertices[1].x + 0.2f * (calculatePolynomial(frame.rightRoadBorder, -20) + 2);
        vertices[2].x = 0.8f * vertices[2].x + 0.2f * (calculatePolynomial(frame.leftRoadBorder, -20) - 2);
        vertices[3].x = 0.8f * vertices[3].x + 0.2f * (calculatePolynomial(frame.leftRoadBorder, 20) - 2);

        // list of index locations for the vertices making up each triangle
        int[] triangles = new int[6];

        triangles[0] = 0;
        triangles[1] = 1;
        triangles[2] = 2;

        triangles[3] = 0;
        triangles[4] = 2;
        triangles[5] = 3;

        Vector2[] uvs = new Vector2[4];
        uvs[0] = new Vector2(1, 1);
        uvs[1] = new Vector2(1, 0);
        uvs[2] = new Vector2(0, 0);
        uvs[3] = new Vector2(0, 1);

        Mesh mesh = new Mesh();
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        mesh.uv = uvs;
        mesh.name = "Road";

        GetComponent<MeshFilter>().mesh = mesh;

        warning = false;

        foreach (var car in cars)
        {
            if (currentFrame == (car.Value.First().Key + (display_threshold ? video_fps / 2 : 0)))
            {
                if (car.Value.ContainsKey(currentFrame))
                {
                    Transform newCar = Instantiate(car_model, new Vector3(car.Value[currentFrame][0], 0.02F, car.Value[currentFrame][1]), Quaternion.Euler(0, 90, 0));
                    newCar.Find("Car/Group19995").GetComponent<MeshRenderer>().materials.First(m => m.name == "Material #32 (Instance)").SetColor("_Color", UnityEngine.Random.ColorHSV(0, 1, 1, 1, 0.5F, 1));
                    carsIG.Add(car.Key, newCar.gameObject);
                }
            }
            else if ((currentFrame >= (car.Value.First().Key + (display_threshold ? video_fps / 2 : 0))) && (currentFrame <= car.Value.Last().Key))
            { 
                var pos = new Vector3(car.Value[currentFrame][0], 0.02F, car.Value[currentFrame][1]);
                carsIG[car.Key].transform.position = pos;                

                if (Vector3.Distance(Vector3.zero, pos) < 10)
                    warning = true;
            }
            else
            {
                if (carsIG.ContainsKey(car.Key))
                {
                    DestroyImmediate(carsIG[car.Key]);
                    carsIG.Remove(car.Key);
                }
            }
        }
    }

    void OnGUI()
    {
        if (warning)
        {
            GUI.DrawTexture(new Rect(10, 10, 42, 42), warning_image, ScaleMode.ScaleToFit, true);
            GUI.Label(new Rect(52, 21, 200, 35), warningText);
        }
    }
}
