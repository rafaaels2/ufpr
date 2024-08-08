Rails.application.routes.draw do
  resources :pessoas
  resources :carros
  resources :computadores
  resources :disciplinas
  resources :turmas
  root "pessoas#index" # Definindo uma rota padrão, ajuste conforme necessário
end

