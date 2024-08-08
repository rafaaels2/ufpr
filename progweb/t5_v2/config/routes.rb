Rails.application.routes.draw do
  root 'pages#home'
  get 'admin_view', to: 'pages#admin_view'
  get 'user_view', to: 'pages#user_view'

  namespace :admin do
    resources :pessoas
  end
end
