import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score
from sklearn.metrics import precision_score
from sklearn.metrics import recall_score
from sklearn.metrics import f1_score

dados = pd.read_csv ('aula2-exemplo.csv')
dados = dados.dropna ()
dados = dados.drop_duplicates ()

X = dados[['Idade', 'Sexo', 'HorasEstudo', 'Prova1']]
X = pd.get_dummies (X, columns=['Sexo'])

df_media = pd.DataFrame ({'media': (dados['Prova1'] + dados['Prova2'])/2})

df_aprovados = pd.DataFrame ({'aprovado': [1 if media >= 6 else 0 for media in df_media['media']]})

y = df_aprovados

y2 = y.values

y2 = y2.flatten ()

X_train, X_test, y_train, y_test = train_test_split (X, y2, test_size = 0.3, random_state = 35)

modelo = LogisticRegression ()
modelo.fit (X_train, y_train)

y_pred = modelo.predict (X_test)
accuracy = accuracy_score (y_test, y_pred)
precision = precision_score (y_test, y_pred)
recall = recall_score (y_test, y_pred)
f1 = f1_score (y_test, y_pred)

print ("Acurácia: {:.2f}".format (accuracy))