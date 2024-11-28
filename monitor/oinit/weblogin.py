from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
 
# open the browser
def fetch_rpcf_code( authUrl:str, clientId : str, redirectUrl:str, scope:str ):
    try:
        driver = webdriver.Firefox()
        targetUrl = authUrl + \
            "?response_type=code&client_id={client_id}&redirect_uri={redirect_uri}&scope={scope}".format(
            client_id=clientId, redirect_uri=redirectUrl, scope=scope)
        driver.get(targetUrl)
        WebDriverWait(driver, 120).until(
            EC.presence_of_element_located((By.ID,"rpcf_load_notify")))
        o = driver.get_cookie("rpcf_code")
        driver.quit()
        return [0,o]
    except Exception as e:
        print(e)
        return [-1, None]