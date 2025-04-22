from locust import HttpUser, task

class StaticWebUser(HttpUser):
    @task
    def index(self):
        self.client.get("/index.html")

    @task
    def page2(self):
        self.client.get("/page2.html")

    @task
    def page3(self):
        self.client.get("/page3.html")

    @task
    def nonexistant(self):
        self.client.get("/nonexistant.html")
